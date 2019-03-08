#pragma once
#include "libplatform/libplatform.h"
#include "v8.h"
#include "platform.h"
#include "v8vm_internal.h"

class V8VirtualMation;

class BaseObject
{
public:
    BaseObject(V8VirtualMation* vm, v8::Local<v8::Object> object);
    virtual ~BaseObject();

    v8::Local<v8::Object> object();
    v8::Local<v8::Object> object(v8::Isolate* isolate);
    Persistent<v8::Object>& persistent() { return m_persistent_handle; }
    void MakeWeak();
    void ClearWeak();

    static BaseObject* FromJSObject(v8::Local<v8::Object> object);
    template <typename T>
    static T* FromJSObject(v8::Local<v8::Object> object);
    static v8::Local<v8::FunctionTemplate> MakeLazilyInitializedJSTemplate(V8VirtualMation* vm);

private:
    BaseObject();
    static void DeleteMe(void* data);

    Persistent<v8::Object> m_persistent_handle;
    V8VirtualMation* m_vm;
};


template <typename T>
inline T* Unwrap(v8::Local<v8::Object> obj) {
    return BaseObject::FromJSObject<T>(obj);
}

#define ASSIGN_OR_RETURN_UNWRAP(ptr, obj, ...)                                                                  \
    *ptr = static_cast<typename std::remove_reference<decltype(*ptr)>::type>(BaseObject::FromJSObject(obj));    \
    if (*ptr == nullptr)                                                                                        \
        return __VA_ARGS__;
