#pragma once
#include "platform.h"

#if defined(_MSC_VER)
#define __V8VM_DECLSPEC_EXPORT  __declspec(dllexport)
#define __V8VM_DECLSPEC_IMPORT  __declspec(dllimport)
#define V8VM_STDCALL __stdcall
#else
#define __V8VM_DECLSPEC_EXPORT  
#define __V8VM_DECLSPEC_IMPORT
#define V8VM_STDCALL 
//#define V8VM_STDCALL __attribute__((__stdcall__))
#endif


#ifdef LIBV8VM
#define V8VM_EXTERN  __V8VM_DECLSPEC_EXPORT
#else
#define V8VM_EXTERN  __V8VM_DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

    V8VM_EXTERN void V8VM_STDCALL InitializeV8Environment();
    V8VM_EXTERN void V8VM_STDCALL ShutdownV8Environment();
    V8VM_EXTERN Int64 V8VM_STDCALL CreateV8VirtualMation(Int64 expected_vmid);
    V8VM_EXTERN void V8VM_STDCALL DisposeV8VirtualMation(Int64 vmid);
    V8VM_EXTERN bool V8VM_STDCALL IsV8VirtualMationInUse(Int64 vmid);
    V8VM_EXTERN bool V8VM_STDCALL IsSmartContractLoaded(Int64 vmid, const char* contract_name);
    V8VM_EXTERN bool V8VM_STDCALL LoadSmartContractBySourcecode(Int64 vmid, const char* contract_name, const char* sourcecode);
    V8VM_EXTERN bool V8VM_STDCALL LoadSmartContractByFileName(Int64 vmid, const char* contract_name, const char* filename);
    V8VM_EXTERN int V8VM_STDCALL InvokeSmartContract(Int64 vmid, const char* contract_name, int param1, const char* param2);

    typedef int(*BalanceTransfer_callback)(Int64 vmid, char* from, char* to, Int64 amount);
    V8VM_EXTERN void V8VM_STDCALL SetBalanceTransfer(BalanceTransfer_callback fn);

    //ZZWTODO 加一个Log回调

#ifdef __cplusplus
}
#endif
