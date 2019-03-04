#include "v8vm_internal.h"
#include "v8vm_util.h"

extern const char* g_js_lib_path;

v8::Isolate* NewIsolate(v8::Isolate::CreateParams* params)
{
    v8::Isolate* isolate = v8::Isolate::New(*params);
    if (isolate == nullptr)
        return nullptr;
    //ZZWTODO 
    //isolate->AddMessageListener(OnMessage);
    //isolate->SetAbortOnUncaughtExceptionCallback(ShouldAbortOnUncaughtException);
    //isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
    //isolate->SetFatalErrorHandler(OnFatalError);
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

    return handle_scope.Escape(result.ToLocalChecked());
}
