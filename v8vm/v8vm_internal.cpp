#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "virtual_mation.h"

extern const char* g_js_lib_path;

void FatalException(v8::Isolate* isolate, v8::Local<v8::Value> error, v8::Local<v8::Message> message)
{
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

void OnMessage(v8::Local<v8::Message> message, v8::Local<v8::Value> error)
{
    FatalException(v8::Isolate::GetCurrent(), error, message);
}

bool ShouldAbortOnUncaughtException(v8::Isolate* isolate)
{
    v8::HandleScope scope(isolate);
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(isolate);
    return vm->inside_should_not_abort_on_uncaught_scope();
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


v8::Isolate* NewIsolate(v8::Isolate::CreateParams* params)
{
    v8::Isolate* isolate = v8::Isolate::New(*params);
    if (isolate == nullptr)
        return nullptr;
    //ZZWTODO 
    isolate->AddMessageListener(OnMessage);
    isolate->SetAbortOnUncaughtExceptionCallback(ShouldAbortOnUncaughtException);
    //isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
    isolate->SetFatalErrorHandler(OnFatalError);
    //isolate->SetAllowWasmCodeGenerationCallback(AllowWasmCodeGenerationCallback);
    return isolate;
}

void OnDisposeIsolate(v8::Isolate* isolate)
{
}

v8::Local<v8::Context> NewContext(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> object_template)
{
    v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, object_template);
    if (context.IsEmpty())
        return context;

    v8::HandleScope handle_scope(isolate);
    context->SetEmbedderData(ContextEmbedderIndex::kAllowWasmCodeGeneration, True(isolate));

    v8::Context::Scope context_scope(context);

    //internal/per_context.js
    LoadScript(isolate, context, "internal/per_context.js");
    //std::string filename(g_js_lib_path);
    //filename += "internal/per_context.js";
    ////Log("per_context: %s\r\n", filename.c_str());
    //char* sourcecode = NULL;
    //bool result = ReadScriptFile(filename.c_str(), sourcecode);
    //int size = strlen(sourcecode);
    //v8::Local<v8::String> per_context = v8::String::NewFromUtf8(isolate, sourcecode, v8::NewStringType::kNormal, static_cast<int>(size)).ToLocalChecked();
    //v8::ScriptCompiler::Source per_context_src(per_context, nullptr);
    //v8::Local<v8::Script> script = v8::ScriptCompiler::Compile(context, &per_context_src).ToLocalChecked();
    //script->Run(context).ToLocalChecked();

    return context;
}

void OnDisposeContext(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    v8::HandleScope handle_scope(isolate);
    v8::Context::Scope context_scope(context);
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, nullptr);
}

v8::MaybeLocal<v8::Value> LoadScript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* script_name)
{
    v8::EscapableHandleScope handle_scope(isolate);

    std::string filename(g_js_lib_path);
    filename += script_name;
    v8::Local<v8::String> filename_v8 = v8::String::NewFromUtf8(isolate, filename.c_str(), v8::NewStringType::kNormal, static_cast<int>(filename.length())).ToLocalChecked();

    char* sourcecode = NULL;
    bool ok = ReadScriptFile(filename.c_str(), sourcecode);
    if (!ok)
    {
        return v8::MaybeLocal<v8::Value>();
    }
    int size = strlen(sourcecode);
    v8::Local<v8::String> sourcecode_v8 = v8::String::NewFromUtf8(isolate, sourcecode, v8::NewStringType::kNormal, static_cast<int>(size)).ToLocalChecked();

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

