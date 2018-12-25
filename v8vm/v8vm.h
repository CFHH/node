#pragma once
#include "platform.h"

#ifdef LIBV8VM
#define V8VM_EXTERN  __declspec(dllexport)
#else
#define V8VM_EXTERN  __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    V8VM_EXTERN void __stdcall InitializeV8Environment();
    V8VM_EXTERN void __stdcall ShutdownV8Environment();
    V8VM_EXTERN Int64 __stdcall CreateV8VirtualMation(Int64 expected_vmid);
    V8VM_EXTERN void __stdcall DisposeV8VirtualMation(Int64 vmid);
    V8VM_EXTERN bool __stdcall IsV8VirtualMationInUse(Int64 vmid);
    V8VM_EXTERN bool __stdcall IsSmartContractLoaded(Int64 vmid, const char* contract_name);
    V8VM_EXTERN bool __stdcall LoadSmartContractBySourcecode(Int64 vmid, const char* contract_name, const char* sourcecode);
    V8VM_EXTERN bool __stdcall LoadSmartContractByFileName(Int64 vmid, const char* contract_name, const char* filename);
    V8VM_EXTERN int __stdcall InvokeSmartContract(Int64 vmid, const char* contract_name, int param1, const char* param2);

    typedef int(*BalanceTransfer_callback)(Int64 vmid, char* from, char* to, Int64 amount);
    V8VM_EXTERN void __stdcall SetBalanceTransfer(BalanceTransfer_callback fn);

#ifdef __cplusplus
}
#endif
