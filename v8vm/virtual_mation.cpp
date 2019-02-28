#include "virtual_mation.h"
#include "v8environment.h"
#include "smart_contract.h"
#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "v8vm_ex.h"

V8VirtualMation::V8VirtualMation(V8Environment* environment, Int64 vmid)
    : m_environment(environment)
    , m_vmid(vmid)
{
    m_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator(); //ZZWTODO 替换为 ArrayBufferAllocator
    m_isolate = NewIsolate(&m_create_params);

    v8::Locker locker(m_isolate);
    v8::Isolate::Scope isolate_scope(m_isolate);
    v8::HandleScope handle_scope(m_isolate);

#define V(PropertyName, StringValue)    \
    m_ ## PropertyName.Set(m_isolate,   \
        v8::Private::New(m_isolate,     \
            v8::String::NewFromOneByte(m_isolate, reinterpret_cast<const uint8_t*>(StringValue), v8::NewStringType::kInternalized, sizeof(StringValue) - 1).ToLocalChecked()));
    PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(V)
#undef V

#define V(PropertyName, StringValue)    \
    m_ ## PropertyName.Set(m_isolate,   \
        v8::Symbol::New(m_isolate,      \
            v8::String::NewFromOneByte(m_isolate, reinterpret_cast<const uint8_t*>(StringValue), v8::NewStringType::kInternalized, sizeof(StringValue) - 1).ToLocalChecked()));
    PER_ISOLATE_SYMBOL_PROPERTIES(V)
#undef V

#define V(PropertyName, StringValue)    \
    m_ ## PropertyName.Set(m_isolate,   \
        v8::String::NewFromOneByte(m_isolate, reinterpret_cast<const uint8_t*>(StringValue), v8::NewStringType::kInternalized, sizeof(StringValue) - 1).ToLocalChecked());
    PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

    //ZZWTODO 实现CommonJS，用require
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(m_isolate);
    global->Set(v8::String::NewFromUtf8(m_isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, Log_JS2C));
    global->Set(v8::String::NewFromUtf8(m_isolate, "BalanceTransfer", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, BalanceTransfer_JS2C));

    v8::Local<v8::Context> context = NewContext(m_isolate, global);
    v8::Context::Scope context_scope(context);
    set_context(context);
    set_as_external(v8::External::New(m_isolate, this));
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, this);

    InstallMap(context, &m_output, "output");
}

V8VirtualMation::~V8VirtualMation()
{
    //Dispose isolate
    {
        v8::Locker locker(m_isolate);
        v8::Isolate::Scope isolate_scope(m_isolate);
        v8::HandleScope handle_scope(m_isolate);
        //Dispose context
        {
            v8::Local<v8::Context> context = this->context();
            OnDisposeContext(m_isolate, context);
        }
        for (std::map<std::string, SmartContract*>::iterator itr = m_contracts.begin(); itr != m_contracts.end(); ++itr)
        {
            SmartContract* contract = itr->second;
            delete contract;
        }
        m_contracts.clear();
        m_invoke_param_template.Reset();
        m_map_template.Reset();

#define V(PropertyName, _) m_ ## PropertyName.Reset();
        ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V

        OnDisposeIsolate(m_isolate);
    }

    m_isolate->Dispose();
    m_isolate = NULL;
    delete m_create_params.array_buffer_allocator; //ZZWTODO 替换为 ArrayBufferAllocator
    m_environment = NULL;
}

bool V8VirtualMation::IsInUse()
{
    return m_isolate->IsInUse();
}

void V8VirtualMation::PumpMessage()
{
    m_environment->PumpMessage(m_isolate);
}

SmartContract* V8VirtualMation::CreateSmartContract(const char* contract_name, const char* sourcecode)
{
    SmartContract* contract = new SmartContract(this);
    if (!contract->Initialize(sourcecode))
    {
        delete contract;
        contract = NULL;
        return NULL;
    }
    m_contracts[contract_name] = contract;
    return contract;
}

void V8VirtualMation::DestroySmartContract(const char* contract_name)
{
    std::map<std::string, SmartContract*>::iterator itr = m_contracts.find(contract_name);
    if (itr == m_contracts.end())
        return;
    SmartContract* contract = itr->second;
    m_contracts.erase(itr);
    delete contract;
}

SmartContract* V8VirtualMation::GetSmartContract(const char* contract_name)
{
    std::map<std::string, SmartContract*>::iterator itr = m_contracts.find(contract_name);
    if (itr == m_contracts.end())
        return NULL;
    SmartContract* contract = itr->second;
    return contract;
}

void V8VirtualMation::InstallMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* stdmap, const char* map_name)
{
    v8::HandleScope handle_scope(m_isolate);
    v8::Local<v8::Object> map_obj = WrapMap(context, stdmap);
    context->Global()->Set(context, v8::String::NewFromUtf8(m_isolate, map_name, v8::NewStringType::kNormal).ToLocalChecked(), map_obj).FromJust();
}

v8::Local<v8::Object> V8VirtualMation::WrapMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* obj)
{
    v8::EscapableHandleScope handle_scope(m_isolate);
    if (m_map_template.IsEmpty())
    {
        v8::Local<v8::ObjectTemplate> map_template = MakeMapTemplate();
        m_map_template.Reset(m_isolate, map_template);
    }
    v8::Local<v8::ObjectTemplate> templ = v8::Local<v8::ObjectTemplate>::New(m_isolate, m_map_template);
    v8::Local<v8::Object> result = templ->NewInstance(context/*m_isolate->GetCurrentContext()*/).ToLocalChecked();
    v8::Local<v8::External> map_ptr = v8::External::New(m_isolate, obj);
    result->SetInternalField(0, map_ptr);
    return handle_scope.Escape(result);
}

v8::Local<v8::ObjectTemplate> V8VirtualMation::MakeMapTemplate()
{
    v8::EscapableHandleScope handle_scope(m_isolate);
    v8::Local<v8::ObjectTemplate> obj_templ = v8::ObjectTemplate::New(m_isolate);
    obj_templ->SetInternalFieldCount(1);
    obj_templ->SetHandler(v8::NamedPropertyHandlerConfiguration(MapGet_JS2C, MapSet_JS2C));
    return handle_scope.Escape(obj_templ);
}



v8::Local<v8::Object> V8VirtualMation::WrapInvokeParam(v8::Local<v8::Context> context, InvokeParam* param)
{
    v8::EscapableHandleScope handle_scope(m_isolate);
    if (m_invoke_param_template.IsEmpty())
    {
        v8::Local<v8::ObjectTemplate> invoke_param_template = MakeInvokeParamTemplate();
        m_invoke_param_template.Reset(m_isolate, invoke_param_template);
    }
    v8::Local<v8::ObjectTemplate> templ = v8::Local<v8::ObjectTemplate>::New(m_isolate, m_invoke_param_template);
    v8::Local<v8::Object> result = templ->NewInstance(context/*m_isolate->GetCurrentContext()*/).ToLocalChecked();
    v8::Local<v8::External> param_ptr = v8::External::New(m_isolate, param);
    result->SetInternalField(0, param_ptr);
    return handle_scope.Escape(result);
}

v8::Local<v8::ObjectTemplate> V8VirtualMation::MakeInvokeParamTemplate()
{
    v8::EscapableHandleScope handle_scope(m_isolate);
    v8::Local<v8::ObjectTemplate> obj_templ = v8::ObjectTemplate::New(m_isolate);
    obj_templ->SetInternalFieldCount(1);
    //obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM0, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam_JS2C);
    //obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM1, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam_JS2C);
    //obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM2, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam_JS2C);
    obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM0, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam0_JS2C);
    obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM1, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam1_JS2C);
    obj_templ->SetAccessor(v8::String::NewFromUtf8(m_isolate, INVOKE_PARAM_PARAM2, v8::NewStringType::kInternalized).ToLocalChecked(), GetInvokeParam2_JS2C);
    return handle_scope.Escape(obj_templ);
}



V8VirtualMation* V8VirtualMation::GetCurrent(v8::Isolate* isolate)
{
    return V8VirtualMation::GetCurrent(isolate->GetCurrentContext());
}

V8VirtualMation* V8VirtualMation::GetCurrent(v8::Local<v8::Context> context)
{
    return static_cast<V8VirtualMation*>(context->GetAlignedPointerFromEmbedderData(ContextEmbedderIndex::kEnvironment));
}

V8VirtualMation* V8VirtualMation::GetCurrent(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    CHECK(info.Data()->IsExternal());
    return static_cast<V8VirtualMation*>(info.Data().As<v8::External>()->Value());
}

template <typename T>
V8VirtualMation* V8VirtualMation::GetCurrent(const v8::PropertyCallbackInfo<T>& info)
{
    CHECK(info.Data()->IsExternal());
    return static_cast<V8VirtualMation*>(info.Data().template As<v8::External>()->Value());
}
