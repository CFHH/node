#include <cstring>
#include "virtual_mation.h"
#include "v8environment.h"
#include "smart_contract.h"
#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "v8vm_ex.h"
#include "vm_module.h"
#include <algorithm>

V8VirtualMation::V8VirtualMation(V8Environment* environment, Int64 vmid)
    : m_environment(environment)
    , m_vmid(vmid)
{
    //创建Isolate做为虚拟机
    m_create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator(); //ZZWTODO 替换为自定义的ArrayBufferAllocator
    m_isolate = NewIsolate(&m_create_params);

    v8::Locker locker(m_isolate);
    v8::Isolate::Scope isolate_scope(m_isolate);
    v8::HandleScope handle_scope(m_isolate);

    //m_isolate->SetData(0, this);

    //定义Isolate的属性
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

    //ZZWTODO COMMONJS 创建V8::Context
    //全局函数
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(m_isolate);
    global->Set(v8::String::NewFromUtf8(m_isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, Log_JS2C));
    //内建模块：sys
    v8::Local<v8::ObjectTemplate> sys_module = v8::ObjectTemplate::New(m_isolate);
    global->Set(v8::String::NewFromUtf8(m_isolate, "sys", v8::NewStringType::kNormal).ToLocalChecked(), sys_module);
    sys_module->Set(v8::String::NewFromUtf8(m_isolate, "LoadScript", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, LoadScript_JS2C));
    //内建模块：fs
    v8::Local<v8::ObjectTemplate> fs_module = v8::ObjectTemplate::New(m_isolate);
    global->Set(v8::String::NewFromUtf8(m_isolate, "fs", v8::NewStringType::kNormal).ToLocalChecked(), fs_module);
    fs_module->Set(v8::String::NewFromUtf8(m_isolate, "IsFileExists", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, IsFileExists_JS2C));
    //内建模块：bc
    v8::Local<v8::ObjectTemplate> bc_module = v8::ObjectTemplate::New(m_isolate);
    global->Set(v8::String::NewFromUtf8(m_isolate, "bc", v8::NewStringType::kNormal).ToLocalChecked(), bc_module);
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "BalanceTransfer", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, BalanceTransfer_JS2C));
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "UpdateDB", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, UpdateDB_JS2C));
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "QueryDB", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, QueryDB_JS2C));
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "RequireAuth", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, RequireAuth_JS2C));
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "GetIntValue", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, GetIntValue_JS2C));
    bc_module->Set(v8::String::NewFromUtf8(m_isolate, "GetStringValue", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(m_isolate, GetStringValue_JS2C));

    v8::Local<v8::Context> context = NewContext(m_isolate, this, global);
    v8::Context::Scope context_scope(context);
    set_context(context);
    set_as_external(v8::External::New(m_isolate, this));
    SetupCommonjs();

    //创建一个process对象
    v8::Local<v8::FunctionTemplate> process_template = v8::FunctionTemplate::New(m_isolate);
    process_template->SetClassName(FIXED_ONE_BYTE_STRING(m_isolate, "process"));
    v8::Local<v8::Object> process_object = process_template->GetFunction()->NewInstance(context).ToLocalChecked();
    set_process_object(process_object);
    SetupProcessObject();

    //LoadEnvironment(); //ZZWTODO COMMONJS NODE 最主要的

    //向JS引入一个std::map，用于测试
    SetupOutputMap(context, &m_output, "output");
}

V8VirtualMation::~V8VirtualMation()
{
    RunCleanup();
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
    delete m_create_params.array_buffer_allocator; //ZZWTODO 替换为自定义的ArrayBufferAllocator
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

void V8VirtualMation::SetupCommonjs()
{
    v8::HandleScope handle_scope(m_isolate);
    v8::Local<v8::Context> context = this->context();
    v8::TryCatch try_catch(m_isolate);
    LoadInternalScript(m_isolate, context, "internal/commonjs.js", false);
    v8::Local<v8::String> runmain_name = v8::String::NewFromUtf8(m_isolate, "runMain", v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::Value> runmain_val;
    context->Global()->Get(context, runmain_name).ToLocal(&runmain_val);
    CHECK(runmain_val->IsFunction());
    v8::Local<v8::Function> runmain_fun = v8::Local<v8::Function>::Cast(runmain_val);
    set_runmain(runmain_fun);
}

void V8VirtualMation::SetupProcessObject()
{
    v8::HandleScope scope(m_isolate);
    v8::Local<v8::Object> process = process_object();
    SetReadOnlyProperty(process, "version", "123456789");
    SetMethod(process, "_log", Log_JS2C);
}

void V8VirtualMation::SetupBootstrapObject(v8::Local<v8::Object> bootstrapper)
{
    //BOOTSTRAP_METHOD(_setupProcessObject, SetupProcessObject);
    //BOOTSTRAP_METHOD(_setupNextTick, SetupNextTick);
    //BOOTSTRAP_METHOD(_setupPromises, SetupPromises);
    //BOOTSTRAP_METHOD(_chdir, Chdir);
    //BOOTSTRAP_METHOD(_cpuUsage, CPUUsage);
    //BOOTSTRAP_METHOD(_hrtime, Hrtime);
    //BOOTSTRAP_METHOD(_hrtimeBigInt, HrtimeBigInt);
    //BOOTSTRAP_METHOD(_memoryUsage, MemoryUsage);
    //BOOTSTRAP_METHOD(_rawDebug, RawDebug);
    //BOOTSTRAP_METHOD(_umask, Umask);

#if defined(__POSIX__) && !defined(__ANDROID__) && !defined(__CloudABI__)
    if (env->is_main_thread()) {
        BOOTSTRAP_METHOD(_initgroups, InitGroups);
        BOOTSTRAP_METHOD(_setegid, SetEGid);
        BOOTSTRAP_METHOD(_seteuid, SetEUid);
        BOOTSTRAP_METHOD(_setgid, SetGid);
        BOOTSTRAP_METHOD(_setuid, SetUid);
        BOOTSTRAP_METHOD(_setgroups, SetGroups);
    }
#endif  // __POSIX__ && !defined(__ANDROID__) && !defined(__CloudABI__)
}

void V8VirtualMation::LoadEnvironment()
{
    v8::HandleScope handle_scope(m_isolate);

    v8::Local<v8::Context> context = this->context();

    v8::TryCatch try_catch(m_isolate);

    //把global暴露给脚本
    v8::Local<v8::Object> global = context->Global();
    global->Set(FIXED_ONE_BYTE_STRING(m_isolate, "global"), global);

    v8::MaybeLocal<v8::Value> v8value = LoadInternalScript(m_isolate, context, "internal/bootstrap/loaders.js", false);
    CHECK(v8value.ToLocalChecked()->IsFunction());
    v8::MaybeLocal<v8::Function> loaders_bootstrapper = v8value.ToLocalChecked().As<v8::Function>();

    v8::Local<v8::Function> get_binding_fn = NewFunctionTemplate(GetBinding)->GetFunction(context).ToLocalChecked();
    v8::Local<v8::Function> get_linked_binding_fn = NewFunctionTemplate(GetLinkedBinding)->GetFunction(context).ToLocalChecked();
    v8::Local<v8::Function> get_internal_binding_fn = NewFunctionTemplate(GetInternalBinding)->GetFunction(context).ToLocalChecked();
    v8::Local<v8::Value> loaders_bootstrapper_args[] = {
        process_object(),
        get_binding_fn,
        get_linked_binding_fn,
        get_internal_binding_fn
    };
    v8::Local<v8::Value> bootstrapped_loaders;
    loaders_bootstrapper.ToLocalChecked()->Call(context, v8::Null(m_isolate), arraysize(loaders_bootstrapper_args), loaders_bootstrapper_args).ToLocal(&bootstrapped_loaders);

    //v8value = LoadInternalScript(m_isolate, context, "internal/bootstrap/v8vm.js", false);
    //CHECK(v8value.ToLocalChecked()->IsFunction());
    //v8::MaybeLocal<v8::Function> v8vm_bootstrapper = v8value.ToLocalChecked().As<v8::Function>();

    //v8::Local<v8::Object> bootstrapper = v8::Object::New(m_isolate);
    //SetupBootstrapObject(bootstrapper);
    //v8::Local<v8::Value> v8vm_bootstrapper_args[] = {
    //    process_object(),
    //    bootstrapper,
    //    bootstrapped_loaders
    //};
    //v8::Local<v8::Object> bootstrapped_v8vm;
    //v8vm_bootstrapper.ToLocalChecked()->Call(context, v8::Null(m_isolate), arraysize(v8vm_bootstrapper_args), v8vm_bootstrapper_args).ToLocal(&bootstrapped_v8vm);
}


/****************************************************************************************************
* 智能合约
****************************************************************************************************/

SmartContract* V8VirtualMation::CreateSmartContractByFileName(const char* contract_name, const char* filename)
{
    SmartContract* contract = new SmartContract(this);
    if (!contract->InitializeByFileName(filename))
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

void V8VirtualMation::SetupOutputMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* stdmap, const char* map_name)
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


/****************************************************************************************************
* 各种方法取得 V8VirtualMation*
****************************************************************************************************/

V8VirtualMation* V8VirtualMation::GetCurrent(v8::Local<v8::Context> context)
{
    //kEnvironment
    return static_cast<V8VirtualMation*>(context->GetAlignedPointerFromEmbedderData(ContextEmbedderIndex::kEnvironment));
}

V8VirtualMation* V8VirtualMation::GetCurrent(v8::Isolate* isolate)
{
    return V8VirtualMation::GetCurrent(isolate->GetCurrentContext());
}

V8VirtualMation* V8VirtualMation::GetCurrent(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    //as_external()
    CHECK(info.Data()->IsExternal());
    return static_cast<V8VirtualMation*>(info.Data().template As<v8::External>()->Value());
}

template <typename T>
V8VirtualMation* V8VirtualMation::GetCurrent(const v8::PropertyCallbackInfo<T>& info)
{
    CHECK(info.Data()->IsExternal());
    return static_cast<V8VirtualMation*>(info.Data().template As<v8::External>()->Value());
}


/****************************************************************************************************
* 设置属性和方法
****************************************************************************************************/

void V8VirtualMation::SetReadOnlyProperty(v8::Local<v8::Object> obj, const char* key, const char* val)
{
    obj->DefineOwnProperty(context(), OneByteString(m_isolate, key), FIXED_ONE_BYTE_STRING(m_isolate, val), v8::ReadOnly).FromJust();
}

v8::Local<v8::FunctionTemplate> V8VirtualMation::NewFunctionTemplate(v8::FunctionCallback callback, v8::Local<v8::Signature> signature, v8::ConstructorBehavior behavior, v8::SideEffectType side_effect_type)
{
    v8::Local<v8::External> external = as_external();
    return v8::FunctionTemplate::New(m_isolate, callback, external, signature, 0, behavior, side_effect_type);
}

void V8VirtualMation::SetMethod(v8::Local<v8::Object> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::Function> function = NewFunctionTemplate(callback, v8::Local<v8::Signature>(), v8::ConstructorBehavior::kAllow, v8::SideEffectType::kHasSideEffect)->GetFunction();
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->Set(name_string, function);
    function->SetName(name_string);
}

void V8VirtualMation::SetMethodNoSideEffect(v8::Local<v8::Object> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::Function> function = NewFunctionTemplate(callback, v8::Local<v8::Signature>(), v8::ConstructorBehavior::kAllow, v8::SideEffectType::kHasNoSideEffect)->GetFunction();
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->Set(name_string, function);
    function->SetName(name_string);
}

void V8VirtualMation::SetProtoMethod(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::Signature> signature = v8::Signature::New(m_isolate, that);
    v8::Local<v8::FunctionTemplate> t = NewFunctionTemplate(callback, signature, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasSideEffect);
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->PrototypeTemplate()->Set(name_string, t);
    t->SetClassName(name_string);
}

void V8VirtualMation::SetProtoMethodNoSideEffect(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::Signature> signature = v8::Signature::New(m_isolate, that);
    v8::Local<v8::FunctionTemplate> t = NewFunctionTemplate(callback, signature, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->PrototypeTemplate()->Set(name_string, t);
    t->SetClassName(name_string);
}

void V8VirtualMation::SetTemplateMethod(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::FunctionTemplate> t = NewFunctionTemplate(callback, v8::Local<v8::Signature>(), v8::ConstructorBehavior::kAllow, v8::SideEffectType::kHasSideEffect);
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->Set(name_string, t);
    t->SetClassName(name_string);
}

void V8VirtualMation::SetTemplateMethodNoSideEffect(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback)
{
    v8::Local<v8::FunctionTemplate> t = NewFunctionTemplate(callback, v8::Local<v8::Signature>(), v8::ConstructorBehavior::kAllow, v8::SideEffectType::kHasNoSideEffect);
    const v8::NewStringType type = v8::NewStringType::kInternalized;
    v8::Local<v8::String> name_string = v8::String::NewFromUtf8(m_isolate, name, type).ToLocalChecked();
    that->Set(name_string, t);
    t->SetClassName(name_string);
}


/****************************************************************************************************
* 异常处理
****************************************************************************************************/

void V8VirtualMation::ThrowError(const char* errmsg)
{
    ThrowError(v8::Exception::Error, errmsg);
}

void V8VirtualMation::ThrowTypeError(const char* errmsg)
{
    ThrowError(v8::Exception::TypeError, errmsg);
}

void V8VirtualMation::ThrowRangeError(const char* errmsg)
{
    ThrowError(v8::Exception::RangeError, errmsg);
}

void V8VirtualMation::ThrowError(v8::Local<v8::Value>(*fun)(v8::Local<v8::String>), const char* errmsg)
{
    v8::HandleScope handle_scope(m_isolate);
    m_isolate->ThrowException(fun(OneByteString(m_isolate, errmsg)));
}

void V8VirtualMation::ReportException(v8::Local<v8::Value> er, v8::Local<v8::Message> message)
{
    CHECK(!er.IsEmpty());
    v8::HandleScope scope(m_isolate);
    v8::Local<v8::Context> context = this->context();
    if (message.IsEmpty())
        message = v8::Exception::CreateMessage(m_isolate, er);
    AppendExceptionLine(er, message, FATAL_ERROR);
    v8::Local<v8::Value> trace_value;
    v8::Local<v8::Value> arrow;
    const bool decorated = IsExceptionDecorated(er);
    if (er->IsUndefined() || er->IsNull())
    {
        trace_value = v8::Undefined(m_isolate);
    }
    else
    {
        v8::Local<v8::Object> err_obj = er->ToObject(context).ToLocalChecked();
        trace_value = err_obj->Get(stack_string());
        arrow = err_obj->GetPrivate(context, arrow_message_private_symbol()).ToLocalChecked();
    }

    v8::String::Utf8Value utf8_trace(m_isolate, trace_value);
    char* trace = *utf8_trace;
    if (trace == nullptr && !trace_value->IsUndefined())
    {
        if (arrow.IsEmpty() || !arrow->IsString() || decorated)
        {
            Log(m_vmid, 0, "%s\n", trace);
        }
        else
        {
            v8::String::Utf8Value utf8_arrow(m_isolate, arrow);
            char* arrow_string = *utf8_arrow;
            Log(m_vmid, 0, "%s\n%s\n", arrow_string, trace);
        }
    }
    else
    {
        // this really only happens for RangeErrors, since they're the only
        // kind that won't have all this info in the trace, or when non-Error
        // objects are thrown manually.
        v8::Local<v8::Value> message;
        v8::Local<v8::Value> name;
        if (er->IsObject())
        {
            v8::Local<v8::Object> err_obj = er.As<v8::Object>();
            message = err_obj->Get(message_string());
            name = err_obj->Get(FIXED_ONE_BYTE_STRING(m_isolate, "name"));
        }
        if (message.IsEmpty() || message->IsUndefined() || name.IsEmpty() || name->IsUndefined())
        {
            v8::String::Utf8Value utf8_message(m_isolate, er);
            char* message = *utf8_message;
            Log(m_vmid, 0, "%s\n", message ? message : "<toString() threw exception>");
        }
        else
        {
            v8::String::Utf8Value utf8_name(m_isolate, name);
            char* name_string = *utf8_name;
            v8::String::Utf8Value utf8_message(m_isolate, message);
            char* message_string = *utf8_message;
            if (arrow.IsEmpty() || !arrow->IsString() || decorated)
            {
                Log(m_vmid, 0, "%s: %s\n", name_string, message_string);
            }
            else
            {
                v8::String::Utf8Value utf8_arrowe(m_isolate, arrow);
                char* arrow_string = *utf8_arrowe;
                Log(m_vmid, 0, "%s\n%s: %s\n", *arrow_string, name_string, message_string);
            }
        }
    }
}

void V8VirtualMation::ReportException(const v8::TryCatch& try_catch)
{
    ReportException(try_catch.Exception(), try_catch.Message());
}

bool V8VirtualMation::IsExceptionDecorated(v8::Local<v8::Value> er)
{
    if (!er.IsEmpty() && er->IsObject())
    {
        v8::Local<v8::Object> err_obj = er.As<v8::Object>();
        auto maybe_value = err_obj->GetPrivate(context(), decorated_private_symbol());
        v8::Local<v8::Value> decorated;
        return maybe_value.ToLocal(&decorated) && decorated->IsTrue();
    }
    return false;
}

void V8VirtualMation::AppendExceptionLine(v8::Local<v8::Value> er, v8::Local<v8::Message> message, enum ErrorHandlingMode mode)
{
    if (message.IsEmpty())
        return;
    v8::Local<v8::Context> context = this->context();

    v8::HandleScope scope(m_isolate);
    v8::Local<v8::Object> err_obj;
    if (!er.IsEmpty() && er->IsObject())
        err_obj = er.As<v8::Object>();

    // Print (filename):(line number): (message).
    v8::ScriptOrigin origin = message->GetScriptOrigin();
    v8::Local<v8::Value> resource_name = message->GetScriptResourceName();
    v8::String::Utf8Value filename(m_isolate, resource_name);
    const char* filename_string = *filename;
    int linenum = message->GetLineNumber(context).FromJust();

    // Print line of source code.
    v8::MaybeLocal<v8::String> source_line_maybe = message->GetSourceLine(context);
    v8::String::Utf8Value sourceline(m_isolate, source_line_maybe.ToLocalChecked());
    const char* sourceline_string = *sourceline;
    if (strstr(sourceline_string, "node-do-not-add-exception-line") != nullptr)
        return;

    // Because of how node modules work, all scripts are wrapped with a
    // "function (module, exports, __filename, ...) {"
    // to provide script local variables.
    //
    // When reporting errors on the first line of a script, this wrapper
    // function is leaked to the user. There used to be a hack here to
    // truncate off the first 62 characters, but it caused numerous other
    // problems when vm.runIn*Context() methods were used for non-module
    // code.
    //
    // If we ever decide to re-instate such a hack, the following steps
    // must be taken:
    //
    // 1. Pass a flag around to say "this code was wrapped"
    // 2. Update the stack frame output so that it is also correct.
    //
    // It would probably be simpler to add a line rather than add some
    // number of characters to the first line, since V8 truncates the
    // sourceline to 78 characters, and we end up not providing very much
    // useful debugging info to the user if we remove 62 characters.

    int script_start = (linenum - origin.ResourceLineOffset()->Value()) == 1 ? origin.ResourceColumnOffset()->Value() : 0;
    int start = message->GetStartColumn(context).FromMaybe(0);
    int end = message->GetEndColumn(context).FromMaybe(0);
    if (start >= script_start)
    {
        CHECK_GE(end, start);
        start -= script_start;
        end -= script_start;
    }

    char arrow[1024];
    int max_off = sizeof(arrow) - 2;
    int off = snprintf(arrow, sizeof(arrow), "%s:%i\n%s\n", filename_string, linenum, sourceline_string);
    CHECK_GE(off, 0);
    if (off > max_off)
        off = max_off;

    // Print wavy underline (GetUnderline is deprecated).
    for (int i = 0; i < start; i++)
    {
        if (sourceline_string[i] == '\0' || off >= max_off)
            break;
        CHECK_LT(off, max_off);
        arrow[off++] = (sourceline_string[i] == '\t') ? '\t' : ' ';
    }
    for (int i = start; i < end; i++)
    {
        if (sourceline_string[i] == '\0' || off >= max_off)
            break;
        CHECK_LT(off, max_off);
        arrow[off++] = '^';
    }
    CHECK_LE(off, max_off);
    arrow[off] = '\n';
    arrow[off + 1] = '\0';

    v8::Local<v8::String> arrow_str = v8::String::NewFromUtf8(m_isolate, arrow);

    const bool can_set_arrow = !arrow_str.IsEmpty() && !err_obj.IsEmpty();
    if (!can_set_arrow || (mode == FATAL_ERROR && !err_obj->IsNativeError()))
    {
        Log(m_vmid, 0, "%s\n", arrow);
        return;
    }
    CHECK(err_obj->SetPrivate(context, arrow_message_private_symbol(), arrow_str).FromMaybe(false));
}


/****************************************************************************************************
* 通过直接传源码的方式创建智能合约时，临时存储
****************************************************************************************************/

bool V8VirtualMation::RegisterSourceCode(std::string& key, const char* sourcecode)
{
    if (m_registered_source_code.find(key) != m_registered_source_code.end())
        return false;
    m_registered_source_code[key] = std::string(sourcecode);
    return true;
}

std::string V8VirtualMation::GetRegisteredSourceCode(std::string& key)
{
    std::map<std::string, std::string>::iterator itr = m_registered_source_code.find(key);
    if (itr == m_registered_source_code.end())
        return std::string("");
    return itr->second;
}

void V8VirtualMation::UnregisterSourceCode(std::string& key)
{
    m_registered_source_code.erase(key);
}

/****************************************************************************************************
* 其他
****************************************************************************************************/

void V8VirtualMation::AddCleanupHook(void(*fn)(void*), void* arg)
{
    auto insertion_info = m_cleanup_hooks.emplace(CleanupHookCallback{fn, arg, m_cleanup_hook_counter++});
    CHECK_EQ(insertion_info.second, true);
}

void V8VirtualMation::RemoveCleanupHook(void(*fn)(void*), void* arg)
{
    CleanupHookCallback search{fn, arg, 0};
    m_cleanup_hooks.erase(search);
}

void V8VirtualMation::RunCleanup()
{
    while (!m_cleanup_hooks.empty())
    {
        std::vector<CleanupHookCallback> callbacks(m_cleanup_hooks.begin(), m_cleanup_hooks.end());
        auto sort_algorithm = [](const CleanupHookCallback& a, const CleanupHookCallback& b)
        {
            return a.m_insertion_order_counter > b.m_insertion_order_counter;
        };
        std::sort(callbacks.begin(), callbacks.end(), sort_algorithm);

        for (const CleanupHookCallback& cb : callbacks)
        {
            if (m_cleanup_hooks.count(cb) == 0)
                continue;
            cb.m_fn(cb.m_arg);
            m_cleanup_hooks.erase(cb);
        }
    }
}
