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
    this->AssignToContext(context);

    v8::Local<v8::FunctionTemplate> process_template = v8::FunctionTemplate::New(m_isolate);
    process_template->SetClassName(FIXED_ONE_BYTE_STRING(m_isolate, "process"));
    v8::Local<v8::Object> process_object = process_template->GetFunction()->NewInstance(context).ToLocalChecked();
    set_process_object(process_object);
    SetupProcessObject();
    LoadEnvironment();

    InstallMap(context, &m_output, "output");
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
    delete m_create_params.array_buffer_allocator; //ZZWTODO 替换为 ArrayBufferAllocator
    m_environment = NULL;
}

void V8VirtualMation::SetupProcessObject()
{
    v8::HandleScope scope(m_isolate);
    v8::Local<v8::Object> process = process_object();
    SetReadOnlyProperty(process, "version", "123456789");
    SetMethod(process, "_log", Log_JS2C);
}

void V8VirtualMation::LoadEnvironment()
{
    v8::HandleScope handle_scope(m_isolate);

    v8::Local<v8::Context> context = this->context();

    v8::TryCatch try_catch(m_isolate);

    v8::Local<v8::Object> global = context->Global();
    global->Set(FIXED_ONE_BYTE_STRING(m_isolate, "global"), global);

    v8::MaybeLocal<v8::Value> bootstrapper = LoadScript(m_isolate, context, "internal/bootstrap/loaders.js");
    CHECK(bootstrapper.ToLocalChecked()->IsFunction());
    v8::MaybeLocal<v8::Function> loaders_bootstrapper = bootstrapper.ToLocalChecked().As<v8::Function>();

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
    //loaders_bootstrapper.ToLocalChecked()->Call(context, v8::Null(m_isolate), arraysize(loaders_bootstrapper_args), loaders_bootstrapper_args).ToLocal(&bootstrapped_loaders);
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



void V8VirtualMation::AssignToContext(v8::Local<v8::Context> context)
{
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, this);
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
        Log("%s\n", arrow);
        return;
    }
    CHECK(err_obj->SetPrivate(context, arrow_message_private_symbol(), arrow_str).FromMaybe(false));
}


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
