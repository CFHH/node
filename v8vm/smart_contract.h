#pragma once

#include <map>
#include "libplatform/libplatform.h"
#include "v8.h"

#define SMART_CONTRACT_PROCESS "Process"
#define INVOKE_PARAM_PARAM1 "param1"
#define INVOKE_PARAM_PARAM2 "param2"

class V8VirtualMation;

struct InvokeParam
{
    int param1;
    std::string param2;
};

class SmartContract
{
public:
    explicit SmartContract(V8VirtualMation* vm);
    ~SmartContract();
    bool Initialize(const char* sourcecode);
    bool Invoke(InvokeParam* param);

private:
    V8VirtualMation* m_vm;
    v8::Global<v8::Context> m_context;
    v8::Global<v8::Script> m_script;
    v8::Global<v8::Function> m_process_fun;
    std::map<std::string, std::string> m_output;
};
