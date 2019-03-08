#include "base_object.h"
#include "virtual_mation.h"

BaseObject::BaseObject(V8VirtualMation* vm, v8::Local<v8::Object> object)
    : m_vm(vm), m_persistent_handle(vm->GetIsolate(), object)
{
    CHECK_EQ(false, object.IsEmpty());
    CHECK_GT(object->InternalFieldCount(), 0);
    object->SetAlignedPointerInInternalField(0, static_cast<void*>(this));
    m_vm->AddCleanupHook(DeleteMe, static_cast<void*>(this));
}

BaseObject::~BaseObject()
{
    m_vm->RemoveCleanupHook(DeleteMe, static_cast<void*>(this));
    if (m_persistent_handle.IsEmpty())
        return;
    v8::HandleScope handle_scope(m_vm->GetIsolate());
    object()->SetAlignedPointerInInternalField(0, nullptr);
}

v8::Local<v8::Object> BaseObject::object()
{
    return PersistentToLocal(m_vm->GetIsolate(), m_persistent_handle);
}

v8::Local<v8::Object> BaseObject::object(v8::Isolate* isolate)
{
    v8::Local<v8::Object> handle = object();
#ifdef DEBUG
    CHECK_EQ(handle->CreationContext()->GetIsolate(), isolate);
    CHECK_EQ(m_vm->GetIsolate(), isolate);
#endif
    return handle;
}

BaseObject* BaseObject::FromJSObject(v8::Local<v8::Object> obj)
{
    CHECK_GT(obj->InternalFieldCount(), 0);
    return static_cast<BaseObject*>(obj->GetAlignedPointerFromInternalField(0));
}

template <typename T>
T* BaseObject::FromJSObject(v8::Local<v8::Object> object)
{
    return static_cast<T*>(FromJSObject(object));
}

void BaseObject::DeleteMe(void* data)
{
    BaseObject* self = static_cast<BaseObject*>(data);
    delete self;
}

void BaseObject::MakeWeak()
{
    auto callback = [](const v8::WeakCallbackInfo<BaseObject>& data)
    {
        BaseObject* obj = data.GetParameter();
        obj->m_persistent_handle.Reset();
        delete obj;
    };
    m_persistent_handle.SetWeak(this, callback, v8::WeakCallbackType::kParameter);
}

void BaseObject::ClearWeak()
{
    m_persistent_handle.ClearWeak();
}

v8::Local<v8::FunctionTemplate> BaseObject::MakeLazilyInitializedJSTemplate(V8VirtualMation* vm)
{
    auto constructor = [](const v8::FunctionCallbackInfo<v8::Value>& args)
    {
#ifdef DEBUG
        CHECK(args.IsConstructCall());
        CHECK_GT(args.This()->InternalFieldCount(), 0);
#endif
        args.This()->SetAlignedPointerInInternalField(0, nullptr);
    };

    v8::Local<v8::FunctionTemplate> t = vm->NewFunctionTemplate(constructor);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    return t;
}
