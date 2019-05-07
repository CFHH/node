#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "virtual_mation.h"

extern const char* g_internal_js_lib_path;
extern const char* g_js_source_path;

const char* g_wrapper[2] = {
    "(function (exports, require, module, __filename, __dirname) {\r\n",
    "\r\n})"
};

bool ShouldAbortOnUncaughtException(v8::Isolate* isolate)
{
    v8::HandleScope scope(isolate);
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(isolate);
    return vm->inside_should_not_abort_on_uncaught_scope();
}

void OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);

    V8VirtualMation* vm = V8VirtualMation::GetCurrent(isolate);
    v8::Local<v8::Object> process_object = vm->process_object();
    v8::Local<v8::String> fatal_exception_string = vm->fatal_exception_string();
    v8::Local<v8::Function> fatal_exception_function = process_object->Get(fatal_exception_string).As<v8::Function>();

    if (!fatal_exception_function->IsFunction())
    {
        vm->ReportException(error, message);
        exit(6);
    }
    else
    {
        v8::TryCatch fatal_try_catch(isolate);
        v8::Local<v8::Value> caught = fatal_exception_function->Call(process_object, 1, &error);
        if (fatal_try_catch.HasTerminated())
            return;
        if (fatal_try_catch.HasCaught())
        {
            vm->ReportException(fatal_try_catch);
            exit(7);
        }
        else if (caught->IsFalse())
        {
            vm->ReportException(error, message);
            exit(1);
        }
    }
}

void OnFatalError(const char* location, const char* message)
{
    if (location)
    {
        Log("FATAL ERROR: %s %s\n", location, message);
    }
    else
    {
        Log("FATAL ERROR: %s\n", message);
    }
    ABORT();
}

void ExitOnPromiseRejectCallback(v8::PromiseRejectMessage promise_reject_message)
{
    //auto isolate = Isolate::GetCurrent();
    //worker* w = (worker*)isolate->GetData(0);
    //assert(w->isolate == isolate);
    //HandleScope handle_scope(w->isolate);
    //auto context = w->context.Get(w->isolate);
    //auto exception = promise_reject_message.GetValue();
    //auto message = Exception::CreateMessage(isolate, exception);
    //auto onerrorStr = String::NewFromUtf8(w->isolate, "onerror");
    //auto onerror = context->Global()->Get(onerrorStr);
    //if (onerror->IsFunction()) {
    //    Local<Function> func = Local<Function>::Cast(onerror);
    //    Local<Value> args[5];
    //    auto origin = message->GetScriptOrigin();
    //    args[0] = exception->ToString();
    //    args[1] = message->GetScriptResourceName();
    //    args[2] = origin.ResourceLineOffset();
    //    args[3] = origin.ResourceColumnOffset();
    //    args[4] = exception;
    //    func->Call(context->Global(), 5, args);
    //    /* message, source, lineno, colno, error */
    //}
    //else {
    //    printf("Unhandled Promise\n");
    //    message->PrintCurrentStackTrace(isolate, stdout);
    //}
    //exit(1);
}


v8::Isolate* NewIsolate(v8::Isolate::CreateParams* params)
{
    v8::Isolate* isolate = v8::Isolate::New(*params);
    if (isolate == nullptr)
        return nullptr;
    ////ZZWTODO COMMONJS NODE
    //isolate->SetCaptureStackTraceForUncaughtExceptions(true);
    //isolate->SetAbortOnUncaughtExceptionCallback(ShouldAbortOnUncaughtException);
    //isolate->AddMessageListener(OnMessage);
    //isolate->SetFatalErrorHandler(OnFatalError);
    ////isolate->SetPromiseRejectCallback(ExitOnPromiseRejectCallback);
    ////isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
    ////isolate->SetAllowWasmCodeGenerationCallback(AllowWasmCodeGenerationCallback);
    return isolate;
}

void OnDisposeIsolate(v8::Isolate* isolate)
{
}

v8::Local<v8::Context> NewContext(v8::Isolate* isolate, V8VirtualMation* vm, v8::Local<v8::ObjectTemplate> object_template)
{
    v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, object_template);
    if (context.IsEmpty())
        return context;
    v8::HandleScope handle_scope(isolate);
    v8::Context::Scope context_scope(context);
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, vm);
    context->SetEmbedderData(ContextEmbedderIndex::kAllowWasmCodeGeneration, True(isolate));

    //internal/per_context.js
    //LoadInternalScript(isolate, context, "internal/per_context.js", false); //ZZWTODO COMMONJS NODE

    return context;
}

void OnDisposeContext(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    v8::HandleScope handle_scope(isolate);
    v8::Context::Scope context_scope(context);
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, nullptr);
}

v8::MaybeLocal<v8::Value> LoadInternalScript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* relative_file_name, bool wrap)
{
    std::string filename(g_internal_js_lib_path);
    filename += relative_file_name;
    return LoadScript(isolate, context, filename, wrap);
}

v8::MaybeLocal<v8::Value> LoadSourceScript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* relative_file_name, bool wrap)
{
    std::string filename(g_js_source_path);
    filename += relative_file_name;
    return LoadScript(isolate, context, filename, wrap);
}

v8::MaybeLocal<v8::Value> LoadScript(v8::Isolate* isolate, v8::Local<v8::Context> context, std::string& absolute_file_name, bool wrap)
{
    v8::EscapableHandleScope handle_scope(isolate);

    v8::Local<v8::String> filename_v8 = v8::String::NewFromUtf8(isolate, absolute_file_name.c_str(), v8::NewStringType::kNormal, static_cast<int>(absolute_file_name.length())).ToLocalChecked();

    std::string sourcecode;
    bool ok = ReadScriptFile(absolute_file_name.c_str(), sourcecode);
    if (!ok)
        return v8::MaybeLocal<v8::Value>();
    if (wrap)
    {
        sourcecode = g_wrapper[0] + sourcecode;
        sourcecode.append(g_wrapper[1]);
    }
    v8::Local<v8::String> sourcecode_v8 = v8::String::NewFromUtf8(isolate, sourcecode.c_str(), v8::NewStringType::kNormal, static_cast<int>(sourcecode.length())).ToLocalChecked();

    v8::TryCatch try_catch(isolate);

    v8::ScriptOrigin origin(filename_v8);

    v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, sourcecode_v8, &origin);
    if (script.IsEmpty())
    {
        ReportV8Exception(isolate, &try_catch);
        return v8::MaybeLocal<v8::Value>();
    }

    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(context);
    if (result.IsEmpty())
    {
        if (try_catch.HasTerminated())
        {
            isolate->CancelTerminateExecution();
            return v8::MaybeLocal<v8::Value>();
        }
        ReportV8Exception(isolate, &try_catch);
        return v8::MaybeLocal<v8::Value>();
    }

    if (try_catch.HasCaught())
    {
        ReportV8Exception(isolate, &try_catch);
        return v8::MaybeLocal<v8::Value>();
    }

    return handle_scope.Escape(result.ToLocalChecked());
}
