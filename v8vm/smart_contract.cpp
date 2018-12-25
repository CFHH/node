#include "smart_contract.h"
#include "virtual_mation.h"
#include "util.h"
#include "v8vmex.h"
#include <string.h>

SmartContract::SmartContract(V8VirtualMation* vm)
    : m_vm(vm)
{
}

SmartContract::~SmartContract()
{
    m_process_fun.Reset();
    m_script.Reset();
    m_context.Reset();
    m_vm = NULL;
}

bool SmartContract::Initialize(const char* sourcecode)
{
    v8::Isolate* isolate = m_vm->GetIsolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    int size = strlen(sourcecode);
    v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, sourcecode, v8::NewStringType::kNormal, static_cast<int>(size)).ToLocalChecked();

    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);

    //ZZWTODO 实现CommonJS，用require
    global->Set(v8::String::NewFromUtf8(isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Log_JS2C));
    global->Set(v8::String::NewFromUtf8(isolate, "BalanceTransfer", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, BalanceTransfer_JS2C));

    v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);
    v8::Context::Scope context_scope(context);

    m_vm->InstallMap(context, &m_output, "output");

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source).ToLocal(&script))
    {
        ReportV8Exception(isolate, &try_catch);
        return false;
    }
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result))
    {
        ReportV8Exception(isolate, &try_catch);
        m_vm->PumpMessage();
        return false;
    }
    m_vm->PumpMessage();

    v8::Local<v8::String> process_name = v8::String::NewFromUtf8(isolate, SMART_CONTRACT_PROCESS, v8::NewStringType::kNormal).ToLocalChecked();
    v8::Local<v8::Value> process_val;
    if (!context->Global()->Get(context, process_name).ToLocal(&process_val) || !process_val->IsFunction()) {
        return false;
    }
    v8::Local<v8::Function> process_fun = v8::Local<v8::Function>::Cast(process_val);

    m_context.Reset(isolate, context);
    m_script.Reset(isolate, script);
    m_process_fun.Reset(isolate, process_fun);

    return true;
}

bool SmartContract::Invoke(InvokeParam* param)
{
    v8::Isolate* isolate = m_vm->GetIsolate();
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate, m_context);
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
        return false;
    }
    m_vm->PumpMessage();
    return true;
}
