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

    //internal/per_context.js
    v8::Context::Scope context_scope(context);
    std::string filename(g_js_lib_path);
    filename += "/internal/per_context.js";
    //Log("per_context: %s\r\n", filename.c_str());
    char* sourcecode = NULL;
    bool result = ReadScriptFile(filename.c_str(), sourcecode);
    int size = strlen(sourcecode);
    v8::Local<v8::String> per_context = v8::String::NewFromUtf8(isolate, sourcecode, v8::NewStringType::kNormal, static_cast<int>(size)).ToLocalChecked();
    v8::ScriptCompiler::Source per_context_src(per_context, nullptr);
    v8::Local<v8::Script> script = v8::ScriptCompiler::Compile(context, &per_context_src).ToLocalChecked();
    script->Run(context).ToLocalChecked();

    return context;
}

void OnDisposeContext(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    v8::HandleScope handle_scope(isolate);
    v8::Context::Scope context_scope(context);
    context->SetAlignedPointerInEmbedderData(ContextEmbedderIndex::kEnvironment, nullptr);
}
