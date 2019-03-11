#include "contextify.h"
#include "vm_module.h"
#include "virtual_mation.h"
#include "v8vm_internal.h"
#include "buffer.h"

v8::Local<v8::Name> Uint32ToName(v8::Local<v8::Context> context, uint32_t index)
{
    return v8::Uint32::New(context->GetIsolate(), index)->ToString(context).ToLocalChecked();
}

ContextifyContext::ContextifyContext(V8VirtualMation* vm, v8::Local<v8::Object> sandbox_obj, const ContextOptions& options)
    : m_vm(vm)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = CreateV8Context(vm, sandbox_obj, options);
    m_context.Reset(isolate, context);

    // Allocation failure or maximum call stack size reached
    if (m_context.IsEmpty())
        return;
    m_context.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
}

// This is an object that just keeps an internal pointer to this
// ContextifyContext.  It's passed to the NamedPropertyHandler.  If we
// pass the main JavaScript context object we're embedded in, then the
// NamedPropertyHandler will store a reference to it forever and keep it
// from getting gc'd.
v8::Local<v8::Value> ContextifyContext::CreateDataWrapper(V8VirtualMation* vm)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::Object> wrapper = vm->script_data_constructor_function()->NewInstance(vm->context()).FromMaybe(v8::Local<v8::Object>());
    if (wrapper.IsEmpty())
        return scope.Escape(v8::Local<v8::Value>::New(isolate, v8::Local<v8::Value>()));
    wrapper->SetAlignedPointerInInternalField(0, this);
    return scope.Escape(wrapper);
}

v8::Local<v8::Context> ContextifyContext::CreateV8Context(V8VirtualMation* vm, v8::Local<v8::Object> sandbox, const ContextOptions& options)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate);

    function_template->SetClassName(sandbox->GetConstructorName());

    v8::Local<v8::ObjectTemplate> object_template = function_template->InstanceTemplate();

    v8::NamedPropertyHandlerConfiguration config(PropertyGetterCallback,
        PropertySetterCallback,
        PropertyDescriptorCallback,
        PropertyDeleterCallback,
        PropertyEnumeratorCallback,
        PropertyDefinerCallback,
        CreateDataWrapper(vm));

    v8::IndexedPropertyHandlerConfiguration indexed_config(
        IndexedPropertyGetterCallback,
        IndexedPropertySetterCallback,
        IndexedPropertyDescriptorCallback,
        IndexedPropertyDeleterCallback,
        PropertyEnumeratorCallback,
        IndexedPropertyDefinerCallback,
        CreateDataWrapper(vm));

    object_template->SetHandler(config);
    object_template->SetHandler(indexed_config);

    v8::Local<v8::Context> new_context = NewContext(isolate, object_template);

    if (new_context.IsEmpty())
    {
        vm->ThrowError("Could not instantiate context");
        return v8::Local<v8::Context>();
    }

    new_context->SetSecurityToken(vm->context()->GetSecurityToken());

    // We need to tie the lifetime of the sandbox object with the lifetime of
    // newly created context. We do this by making them hold references to each
    // other. The context can directly hold a reference to the sandbox as an
    // embedder data field. However, we cannot hold a reference to a v8::Context
    // directly in an Object, we instead hold onto the new context's global
    // object instead (which then has a reference to the context).
    new_context->SetEmbedderData(ContextEmbedderIndex::kSandboxObject, sandbox);
    sandbox->SetPrivate(vm->context(), vm->contextify_global_private_symbol(), new_context->Global());

    new_context->AllowCodeGenerationFromStrings(options.allow_code_gen_strings->IsTrue());
    new_context->SetEmbedderData(ContextEmbedderIndex::kAllowWasmCodeGeneration, options.allow_code_gen_wasm);
    vm->AssignToContext(new_context);

    return scope.Escape(new_context);
}

v8::Local<v8::Context> ContextifyContext::context() const
{
    return PersistentToLocal(m_vm->GetIsolate(), m_context);
}

ContextifyContext* ContextifyContext::ContextFromContextifiedSandbox(V8VirtualMation* vm, const v8::Local<v8::Object>& sandbox)
{
    v8::MaybeLocal<v8::Value> maybe_value = sandbox->GetPrivate(vm->context(), vm->contextify_context_private_symbol());
    v8::Local<v8::Value> context_external_v;
    if (maybe_value.ToLocal(&context_external_v) && context_external_v->IsExternal())
    {
        v8::Local<v8::External> context_external = context_external_v.As<v8::External>();
        return static_cast<ContextifyContext*>(context_external->Value());
    }
    return nullptr;
}

template <typename T>
ContextifyContext* ContextifyContext::Get(const v8::PropertyCallbackInfo<T>& args)
{
    v8::Local<v8::Value> data = args.Data();
    return static_cast<ContextifyContext*>(data.As<v8::Object>()->GetAlignedPointerFromInternalField(0));
}

void ContextifyContext::InitializeContextifyModule(V8VirtualMation* vm, v8::Local<v8::Object> target)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate);
    function_template->InstanceTemplate()->SetInternalFieldCount(1);
    vm->set_script_data_constructor_function(function_template->GetFunction());
    vm->SetMethod(target, "makeContext", MakeContext);
    vm->SetMethod(target, "isContext", IsContext);
}

// makeContext(v8::Object sandbox, string name, string origin, bool allow_code_gen_strings, bool allow_code_gen_wasm);
void ContextifyContext::MakeContext(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK_EQ(args.Length(), 5);
    CHECK(args[0]->IsObject());
    v8::Local<v8::Object> sandbox = args[0].As<v8::Object>();
    // Don't allow contextifying a sandbox multiple times.
    CHECK(!sandbox->HasPrivate(context, vm->contextify_context_private_symbol()).FromJust());

    ContextOptions options;

    CHECK(args[1]->IsString());
    options.name = args[1].As<v8::String>();

    CHECK(args[2]->IsString() || args[2]->IsUndefined());
    if (args[2]->IsString())
    {
        options.origin = args[2].As<v8::String>();
    }

    CHECK(args[3]->IsBoolean());
    options.allow_code_gen_strings = args[3].As<v8::Boolean>();

    CHECK(args[4]->IsBoolean());
    options.allow_code_gen_wasm = args[4].As<v8::Boolean>();

    v8::TryCatch try_catch(isolate);
    ContextifyContext* ctx = new ContextifyContext(vm, sandbox, options);

    if (try_catch.HasCaught())
    {
        try_catch.ReThrow();
        return;
    }

    if (ctx->context().IsEmpty())
        return;

    sandbox->SetPrivate(context, vm->contextify_context_private_symbol(), v8::External::New(isolate, ctx));
}

void ContextifyContext::IsContext(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK(args[0]->IsObject());
    v8::Local<v8::Object> sandbox = args[0].As<v8::Object>();

    v8::Maybe<bool> result = sandbox->HasPrivate(context, vm->contextify_context_private_symbol());
    args.GetReturnValue().Set(result.FromJust());
}

void ContextifyContext::WeakCallback(const v8::WeakCallbackInfo<ContextifyContext>& data)
{
    ContextifyContext* context = data.GetParameter();
    delete context;
}

void ContextifyContext::PropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::Local<v8::Context> context = ctx->context();
    v8::Local<v8::Object> sandbox = ctx->sandbox();
    v8::MaybeLocal<v8::Value> maybe_rv = sandbox->GetRealNamedProperty(context, property);
    if (maybe_rv.IsEmpty())
    {
        maybe_rv = ctx->global_proxy()->GetRealNamedProperty(context, property);
    }

    v8::Local<v8::Value> rv;
    if (maybe_rv.ToLocal(&rv))
    {
        if (rv == sandbox)
            rv = ctx->global_proxy();
        args.GetReturnValue().Set(rv);
    }
}

void ContextifyContext::PropertySetterCallback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::PropertyAttribute attributes = v8::PropertyAttribute::None;
    bool is_declared_on_global_proxy = ctx->global_proxy()->GetRealNamedPropertyAttributes(ctx->context(), property).To(&attributes);
    bool read_only = static_cast<int>(attributes) & static_cast<int>(v8::PropertyAttribute::ReadOnly);

    bool is_declared_on_sandbox = ctx->sandbox()->GetRealNamedPropertyAttributes(ctx->context(), property).To(&attributes);
    read_only = read_only || (static_cast<int>(attributes) & static_cast<int>(v8::PropertyAttribute::ReadOnly));
    if (read_only)
        return;

    // true for x = 5
    // false for this.x = 5
    // false for Object.defineProperty(this, 'foo', ...)
    // false for vmResult.x = 5 where vmResult = vm.runInContext();
    bool is_contextual_store = ctx->global_proxy() != args.This();

    // Indicator to not return before setting (undeclared) function declarations
    // on the sandbox in strict mode, i.e. args.ShouldThrowOnError() = true.
    // True for 'function f() {}', 'this.f = function() {}',
    // 'var f = function()'.
    // In effect only for 'function f() {}' because
    // var f = function(), is_declared = true
    // this.f = function() {}, is_contextual_store = false.
    bool is_function = value->IsFunction();

    bool is_declared = is_declared_on_global_proxy || is_declared_on_sandbox;
    if (!is_declared && args.ShouldThrowOnError() && is_contextual_store && !is_function)
        return;

    if (!is_declared_on_global_proxy && is_declared_on_sandbox  && args.ShouldThrowOnError() && is_contextual_store && !is_function)
    {
        // The property exists on the sandbox but not on the global
        // proxy. Setting it would throw because we are in strict mode.
        // Don't attempt to set it by signaling that the call was
        // intercepted. Only change the value on the sandbox.
        args.GetReturnValue().Set(false);
    }

    ctx->sandbox()->Set(property, value);
}

void ContextifyContext::PropertyDescriptorCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::Local<v8::Context> context = ctx->context();
    v8::Local<v8::Object> sandbox = ctx->sandbox();

    if (sandbox->HasOwnProperty(context, property).FromMaybe(false))
        args.GetReturnValue().Set(sandbox->GetOwnPropertyDescriptor(context, property).ToLocalChecked());
}

void ContextifyContext::PropertyDefinerCallback(v8::Local<v8::Name> property, const v8::PropertyDescriptor& desc, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::Local<v8::Context> context = ctx->context();
    v8::Isolate* isolate = context->GetIsolate();

    v8::PropertyAttribute attributes = v8::PropertyAttribute::None;
    bool is_declared = ctx->global_proxy()->GetRealNamedPropertyAttributes(ctx->context(), property).To(&attributes);
    bool read_only = static_cast<int>(attributes) & static_cast<int>(v8::PropertyAttribute::ReadOnly);

    // If the property is set on the global as read_only, don't change it on the global or sandbox.
    if (is_declared && read_only)
        return;

    v8::Local<v8::Object> sandbox = ctx->sandbox();

    auto define_prop_on_sandbox = [&](v8::PropertyDescriptor* desc_for_sandbox)
    {
        if (desc.has_enumerable())
            desc_for_sandbox->set_enumerable(desc.enumerable());
        if (desc.has_configurable())
            desc_for_sandbox->set_configurable(desc.configurable());
        // Set the property on the sandbox.
        sandbox->DefineProperty(context, property, *desc_for_sandbox).FromJust();
    };

    if (desc.has_get() || desc.has_set())
    {
        v8::PropertyDescriptor desc_for_sandbox(desc.has_get() ? desc.get() : Undefined(isolate).As<v8::Value>(),
            desc.has_set() ? desc.set() : Undefined(isolate).As<v8::Value>());
        define_prop_on_sandbox(&desc_for_sandbox);
    }
    else
    {
        v8::Local<v8::Value> value = desc.has_value() ? desc.value() : Undefined(isolate).As<v8::Value>();
        if (desc.has_writable())
        {
            v8::PropertyDescriptor desc_for_sandbox(value, desc.writable());
            define_prop_on_sandbox(&desc_for_sandbox);
        }
        else
        {
            v8::PropertyDescriptor desc_for_sandbox(value);
            define_prop_on_sandbox(&desc_for_sandbox);
        }
    }
}

void ContextifyContext::PropertyDeleterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::Maybe<bool> success = ctx->sandbox()->Delete(ctx->context(), property);
    if (success.FromMaybe(false))
        return;

    // Delete failed on the sandbox, intercept and do not delete on the global object.
    args.GetReturnValue().Set(false);
}

void ContextifyContext::PropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;
    args.GetReturnValue().Set(ctx->sandbox()->GetPropertyNames());
}

void ContextifyContext::IndexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;
    ContextifyContext::PropertyGetterCallback(Uint32ToName(ctx->context(), index), args);
}

void ContextifyContext::IndexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;
    ContextifyContext::PropertySetterCallback(Uint32ToName(ctx->context(), index), value, args);
}

void ContextifyContext::IndexedPropertyDescriptorCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;
    ContextifyContext::PropertyDescriptorCallback(Uint32ToName(ctx->context(), index), args);
}

void ContextifyContext::IndexedPropertyDefinerCallback(uint32_t index, const v8::PropertyDescriptor& desc, const v8::PropertyCallbackInfo<v8::Value>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;
    ContextifyContext::PropertyDefinerCallback(Uint32ToName(ctx->context(), index), desc, args);
}

void ContextifyContext::IndexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& args)
{
    ContextifyContext* ctx = ContextifyContext::Get(args);
    if (ctx->m_context.IsEmpty())
        return;

    v8::Maybe<bool> success = ctx->sandbox()->Delete(ctx->context(), index);

    if (success.FromMaybe(false))
        return;

    // Delete failed on the sandbox, intercept and do not delete on the global object.
    args.GetReturnValue().Set(false);
}






ContextifyScript::ContextifyScript(V8VirtualMation* vm, v8::Local<v8::Object> object)
    : BaseObject(vm, object)
{
    MakeWeak();
}

// new ContextifyScript(code, filename, lineOffset, columnOffset, cachedData, produceCachedData, parsingContext)
void ContextifyScript::New(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK(args.IsConstructCall());

    const int argc = args.Length();
    CHECK_GE(argc, 2);

    CHECK(args[0]->IsString());
    v8::Local<v8::String> code = args[0].As<v8::String>();

    CHECK(args[1]->IsString());
    v8::Local<v8::String> filename = args[1].As<v8::String>();

    v8::Local<v8::Integer> line_offset;
    v8::Local<v8::Integer> column_offset;
    v8::Local<v8::Uint8Array> cached_data_buf;
    bool produce_cached_data = false;
    v8::Local<v8::Context> parsing_context = context;

    if (argc > 2)
    {        
        CHECK_EQ(argc, 7);

        CHECK(args[2]->IsNumber());
        line_offset = args[2].As<v8::Integer>();

        CHECK(args[3]->IsNumber());
        column_offset = args[3].As<v8::Integer>();

        if (!args[4]->IsUndefined())
        {
            CHECK(args[4]->IsUint8Array());
            cached_data_buf = args[4].As<v8::Uint8Array>();
        }

        CHECK(args[5]->IsBoolean());
        produce_cached_data = args[5]->IsTrue();

        if (!args[6]->IsUndefined())
        {
            CHECK(args[6]->IsObject());
            ContextifyContext* sandbox = ContextifyContext::ContextFromContextifiedSandbox(vm, args[6].As<v8::Object>());
            CHECK_NOT_NULL(sandbox);
            parsing_context = sandbox->context();
        }
    }
    else
    {
        line_offset = v8::Integer::New(isolate, 0);
        column_offset = v8::Integer::New(isolate, 0);
    }

    ContextifyScript* contextify_script = new ContextifyScript(vm, args.This());

    v8::ScriptCompiler::CachedData* cached_data = nullptr;
    if (!cached_data_buf.IsEmpty())
    {
        v8::ArrayBuffer::Contents contents = cached_data_buf->Buffer()->GetContents();
        uint8_t* data = static_cast<uint8_t*>(contents.Data());
        cached_data = new v8::ScriptCompiler::CachedData(data + cached_data_buf->ByteOffset(), cached_data_buf->ByteLength());
    }

    v8::ScriptOrigin origin(filename, line_offset, column_offset);
    v8::ScriptCompiler::Source source(code, origin, cached_data);
    v8::ScriptCompiler::CompileOptions compile_options = v8::ScriptCompiler::kNoCompileOptions;

    if (source.GetCachedData() != nullptr)
        compile_options = v8::ScriptCompiler::kConsumeCodeCache;

    v8::TryCatch try_catch(isolate);
    V8VirtualMation::ShouldNotAbortOnUncaughtScope no_abort_scope(vm);
    v8::Context::Scope scope(parsing_context);

    v8::MaybeLocal<v8::UnboundScript> v8_script = v8::ScriptCompiler::CompileUnboundScript(isolate, &source, compile_options);

    if (v8_script.IsEmpty())
    {
        DecorateErrorStack(vm, try_catch);
        no_abort_scope.Close();
        try_catch.ReThrow();
        return;
    }
    contextify_script->m_script.Reset(isolate, v8_script.ToLocalChecked());

    if (compile_options == v8::ScriptCompiler::kConsumeCodeCache)
    {
        args.This()->Set(vm->cached_data_rejected_string(), v8::Boolean::New(isolate, source.GetCachedData()->rejected));
    }
    else if (produce_cached_data)
    {
        const v8::ScriptCompiler::CachedData* cached_data = v8::ScriptCompiler::CreateCodeCache(v8_script.ToLocalChecked());
        bool cached_data_produced = cached_data != nullptr;
        if (cached_data_produced)
        {
            v8::MaybeLocal<v8::Object> buf = Buffer::Copy(vm, reinterpret_cast<const char*>(cached_data->data), cached_data->length);
            args.This()->Set(vm->cached_data_string(), buf.ToLocalChecked());
        }
        args.This()->Set(vm->cached_data_produced_string(), v8::Boolean::New(isolate, cached_data_produced));
    }
}

bool ContextifyScript::InstanceOf(V8VirtualMation* vm, const v8::Local<v8::Value>& value)
{
    return !value.IsEmpty() && vm->script_context_constructor_template()->HasInstance(value);
}

void ContextifyScript::CreateCachedData(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    ContextifyScript* wrapped_script;
    ASSIGN_OR_RETURN_UNWRAP(&wrapped_script, args.Holder());
    v8::Local<v8::UnboundScript> unbound_script = PersistentToLocal(isolate, wrapped_script->m_script);
    std::unique_ptr<v8::ScriptCompiler::CachedData> cached_data(v8::ScriptCompiler::CreateCodeCache(unbound_script));
    if (!cached_data)
    {
        args.GetReturnValue().Set(Buffer::New(vm, 0).ToLocalChecked());
    }
    else
    {
        v8::MaybeLocal<v8::Object> buf = Buffer::Copy(vm, reinterpret_cast<const char*>(cached_data->data), cached_data->length);
        args.GetReturnValue().Set(buf.ToLocalChecked());
    }
}

void ContextifyScript::RunInThisContext(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    ContextifyScript* wrapped_script;
    ASSIGN_OR_RETURN_UNWRAP(&wrapped_script, args.Holder());

    CHECK_EQ(args.Length(), 3);

    CHECK(args[0]->IsNumber());
    int64_t timeout = args[0]->IntegerValue(vm->context()).FromJust();

    CHECK(args[1]->IsBoolean());
    bool display_errors = args[1]->IsTrue();

    CHECK(args[2]->IsBoolean());
    bool break_on_sigint = args[2]->IsTrue();

    // Do the eval within this context
    EvalMachine(vm, timeout, display_errors, break_on_sigint, args);
}

void ContextifyScript::RunInContext(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    ContextifyScript* wrapped_script;
    ASSIGN_OR_RETURN_UNWRAP(&wrapped_script, args.Holder());

    CHECK_EQ(args.Length(), 4);

    CHECK(args[0]->IsObject());
    v8::Local<v8::Object> sandbox = args[0].As<v8::Object>();
    ContextifyContext* contextify_context = ContextifyContext::ContextFromContextifiedSandbox(vm, sandbox);
    CHECK_NOT_NULL(contextify_context);
    if (contextify_context->context().IsEmpty())
        return;

    CHECK(args[1]->IsNumber());
    int64_t timeout = args[1]->IntegerValue(vm->context()).FromJust();

    CHECK(args[2]->IsBoolean());
    bool display_errors = args[2]->IsTrue();

    CHECK(args[3]->IsBoolean());
    bool break_on_sigint = args[3]->IsTrue();

    v8::Context::Scope context_scope(contextify_context->context());
    EvalMachine(contextify_context->vm(), timeout, display_errors, break_on_sigint, args);
}

void ContextifyScript::DecorateErrorStack(V8VirtualMation* vm, const v8::TryCatch& try_catch)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();
    v8::Local<v8::Value> exception = try_catch.Exception();
    if (!exception->IsObject())
        return;

    v8::Local<v8::Object> err_obj = exception.As<v8::Object>();
    if (vm->IsExceptionDecorated(err_obj))
        return;

    vm->AppendExceptionLine(exception, try_catch.Message(), CONTEXTIFY_ERROR);

    v8::Local<v8::Value> stack = err_obj->Get(vm->stack_string());
    if (stack.IsEmpty() || !stack->IsString()) {
        return;
    }

    v8::MaybeLocal<v8::Value> maybe_value = err_obj->GetPrivate(context, vm->arrow_message_private_symbol());
    v8::Local<v8::Value> arrow;
    if (!(maybe_value.ToLocal(&arrow) && arrow->IsString())) {
        return;
    }

    v8::Local<v8::String> decorated_stack = v8::String::Concat(v8::String::Concat(arrow.As<v8::String>(), FIXED_ONE_BYTE_STRING(isolate, "\n")), stack.As<v8::String>());
    err_obj->Set(vm->stack_string(), decorated_stack);
    err_obj->SetPrivate(context, vm->decorated_private_symbol(), v8::True(isolate));
}

bool ContextifyScript::EvalMachine(V8VirtualMation* vm, const int64_t timeout, const bool display_errors, const bool break_on_sigint, const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!vm->CanCallIntoJS())
        return false;
    if (!ContextifyScript::InstanceOf(vm, args.Holder()))
    {
        vm->ThrowTypeError( "Script methods can only be called on script instances.");
        return false;
    }
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();
    v8::TryCatch try_catch(isolate);
    ContextifyScript* wrapped_script;
    ASSIGN_OR_RETURN_UNWRAP(&wrapped_script, args.Holder(), false);
    v8::Local<v8::UnboundScript> unbound_script = PersistentToLocal(isolate, wrapped_script->m_script);
    v8::Local<v8::Script> script = unbound_script->BindToCurrentContext();

    v8::MaybeLocal<v8::Value> result = script->Run(context);
    if (try_catch.HasCaught())
    {
        if (display_errors)
            DecorateErrorStack(vm, try_catch);
        // If there was an exception thrown during script execution, re-throw it.
        // If one of the above checks threw, re-throw the exception instead of
        // letting try_catch catch it.
        // If execution has been terminated, but not by one of the watchdogs from
        // this invocation, this will re-throw a `null` value.
        try_catch.ReThrow();
        return false;
    }

    args.GetReturnValue().Set(result.ToLocalChecked());
    return true;
}

void ContextifyScript::InitializeContextifyModule(V8VirtualMation* vm, v8::Local<v8::Object> target)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::String> class_name = FIXED_ONE_BYTE_STRING(isolate, "ContextifyScript");

    v8::Local<v8::FunctionTemplate> script_tmpl = vm->NewFunctionTemplate(New);
    script_tmpl->InstanceTemplate()->SetInternalFieldCount(1);
    script_tmpl->SetClassName(class_name);
    vm->SetProtoMethod(script_tmpl, "createCachedData", CreateCachedData);
    vm->SetProtoMethod(script_tmpl, "runInContext", RunInContext);
    vm->SetProtoMethod(script_tmpl, "runInThisContext", RunInThisContext);

    target->Set(class_name, script_tmpl->GetFunction());
    vm->set_script_context_constructor_template(script_tmpl);

    v8::Local<v8::Symbol> parsing_context_symbol = v8::Symbol::New(isolate, FIXED_ONE_BYTE_STRING(isolate, "script parsing context"));
    vm->set_vm_parsing_context_symbol(parsing_context_symbol);
    target->Set(vm->context(), FIXED_ONE_BYTE_STRING(isolate, "kParsingContext"), parsing_context_symbol).FromJust();
}



void InitializeContextifyModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    ContextifyContext::InitializeContextifyModule(vm, exports);
    ContextifyScript::InitializeContextifyModule(vm, exports);
}

NODE_BUILTIN_MODULE_CONTEXT_AWARE(contextify, InitializeContextifyModule)
