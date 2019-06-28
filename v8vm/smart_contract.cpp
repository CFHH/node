#include "smart_contract.h"
#include "virtual_mation.h"
#include "v8vm_util.h"
#include <string.h>

SmartContract::SmartContract(V8VirtualMation* vm)
    : m_vm(vm)
{
}

SmartContract::~SmartContract()
{
    m_process_fun.Reset();
#if SMART_CONTRACT_OWN_SEPERATE_CONTEXT
    m_context.Reset();
#endif
    m_vm = NULL;
}

bool SmartContract::InitializeByFileName(const char* filename)
{
    v8::Isolate* isolate = m_vm->GetIsolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

#if SMART_CONTRACT_OWN_SEPERATE_CONTEXT
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    //ZZWTODO COMMONJS
    global->Set(v8::String::NewFromUtf8(isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Log_JS2C));
    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
    v8::Context::Scope context_scope(context);
    m_vm->SetupOutputMap(context, &m_output, "output");
#else
    v8::Local<v8::Context> context = m_vm->context();
    v8::Context::Scope context_scope(context);
#endif

    //调用runMain
    v8::Local<v8::String> v8filename = v8::String::NewFromUtf8(isolate, filename, v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::Value> args[] = {
        v8filename,
    };
    v8::Local<v8::Function> runmain = m_vm->runmain();
    v8::MaybeLocal<v8::Value> maybe_exports = runmain->Call(context, v8::Null(isolate), arraysize(args), args);
    //获得exports
    v8::Local<v8::Value> local_exports;
    maybe_exports.ToLocal(&local_exports);
    if (!local_exports->IsObject())
        return false;
    v8::Local<v8::Object> exports = v8::Local<v8::Object>::Cast(local_exports);
    //获得exports.process函数
    v8::Local<v8::String> process_name = v8::String::NewFromUtf8(isolate, SMART_CONTRACT_PROCESS, v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::Value> process_val;
    if (!exports->Get(context, process_name).ToLocal(&process_val) || !process_val->IsFunction()) {
        return false;
    }
    v8::Local<v8::Function> process_fun = v8::Local<v8::Function>::Cast(process_val);
    
#if SMART_CONTRACT_OWN_SEPERATE_CONTEXT
    m_context.Reset(isolate, context);
#endif
    m_process_fun.Reset(isolate, process_fun);
    return true;
}

int SmartContract::Invoke(InvokeParam* param)
{
    v8::Isolate* isolate = m_vm->GetIsolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
#if SMART_CONTRACT_OWN_SEPERATE_CONTEXT
    v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, m_context);
#else
    v8::Local<v8::Context> context = m_vm->context();
#endif
    v8::Context::Scope context_scope(context);

    v8::Local<v8::Object> param_obj = m_vm->WrapInvokeParam(context, param);
    v8::TryCatch try_catch(isolate);
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { param_obj };
    v8::Local<v8::Function> process = v8::Local<v8::Function>::New(isolate, m_process_fun);
    v8::Local<v8::Value> result;
    if (!process->Call(context, context->Global(), argc, argv).ToLocal(&result))
    {
        ReportV8Exception(isolate, &try_catch);
        m_vm->PumpMessage();
        return -20000;
    }
    m_vm->PumpMessage();

    if (result->IsInt32())
        return result->Int32Value();
    else if (result->IsNullOrUndefined())
        return 0;
    else
        return -20001;
    return 0;
}
