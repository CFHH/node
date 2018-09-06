#pragma once

#include <map>
#include "libplatform/libplatform.h"
#include "v8.h"

class V8Environment;
class SmartContract;
struct InvokeParam;

class V8VirtualMation
{
public:
    explicit V8VirtualMation(V8Environment* environment, __int64 vmid);
    ~V8VirtualMation();
    v8::Isolate* GetIsolate() { return m_isolate; }
    __int64 VMID() { return m_vmid; }
    void PumpMessage();

    SmartContract* CreateSmartContract(const char* contract_name, const char* sourcecode);
    void DestroySmartContract(const char* contract_name);
    SmartContract* GetSmartContract(const char* contract_name);

    void InstallMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* stdmap, const char* map_name);

    v8::Local<v8::Object> V8VirtualMation::WrapInvokeParam(v8::Local<v8::Context> context, InvokeParam* param);

private:
    v8::Local<v8::Object> WrapMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* obj);
    v8::Local<v8::ObjectTemplate> MakeMapTemplate();
    v8::Local<v8::ObjectTemplate> MakeInvokeParamTemplate();

private:
    V8Environment* m_environment;
    __int64 m_vmid;
    v8::Isolate::CreateParams m_create_params;
    v8::Isolate* m_isolate;
    v8::Global<v8::ObjectTemplate> m_map_template;
    v8::Global<v8::ObjectTemplate> m_invoke_param_template;
    std::map<std::string, SmartContract*> m_contracts;
};
