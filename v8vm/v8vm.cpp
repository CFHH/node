#include "v8vm.h"
#include "v8environment.h"
#include "virtual_mation.h"
#include "smart_contract.h"
#include "util.h"

V8Environment* g_environment = NULL;

V8VM_EXPORT void __stdcall InitializeEnvironment()
{
    if (g_environment != NULL)
        return;
    g_environment = new V8Environment();
}

V8VM_EXPORT void __stdcall ShutdownEnvironment()
{
    if (g_environment == NULL)
        return;
    delete g_environment;
    g_environment = NULL;
}

V8VM_EXPORT __int64 __stdcall CreateVirtualMation()
{
    if (g_environment != NULL)
        return 0;
    V8VirtualMation* vm = g_environment->CreateVirtualMation();
    if (vm == NULL)
        return 0;
    return vm->VMID();
}

V8VM_EXPORT void __stdcall DisposeVirtualMation(__int64 vmid)
{
    if (g_environment != NULL)
        return;
    g_environment->DisposeVirtualMation(vmid);
}

V8VM_EXPORT bool __stdcall IsSmartContractLoaded(__int64 vmid, const char* contract_name)
{
    if (g_environment != NULL)
        return false;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return false;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract == NULL)
        return false;
    return true;
}

V8VM_EXPORT bool __stdcall LoadSmartContractBySourcecode(__int64 vmid, const char* contract_name, const char* sourcecode)
{
    if (g_environment != NULL)
        return false;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return false;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract != NULL)
        return false;
    contract = vm->CreateSmartContract(contract_name, sourcecode);
    if (contract == NULL)
        return false;
    return true;
}

V8VM_EXPORT bool __stdcall LoadSmartContractByFileName(__int64 vmid, const char* contract_name, const char* filename)
{
    char* buf = NULL;
    bool result = ReadScriptFile(filename, buf);
    if (!result)
    {
        delete buf;
        return false;
    }
    result = LoadSmartContractBySourcecode(vmid, contract_name, buf);
    delete buf;
    return result;
}

V8VM_EXPORT int __stdcall InvokeSmartContract(__int64 vmid, const char* contract_name, int param1, const char* param2)
{
    if (g_environment != NULL)
        return -1;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return -1;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract == NULL)
        return -1;

    InvokeParam param;
    param.param1 = param1;
    param.param2 = param2;
    bool result = contract->Invoke(&param);
    if (!result)
        return -1;
    return 0;
}
