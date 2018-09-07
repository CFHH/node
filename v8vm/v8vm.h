#pragma once

#ifdef LIBV8VM
#define V8VM_EXPORT  __declspec(dllexport)
#else
#define V8VM_EXPORT  __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	V8VM_EXPORT void __stdcall InitializeV8Environment();
	V8VM_EXPORT void __stdcall ShutdownV8Environment();
	V8VM_EXPORT __int64 __stdcall CreateV8VirtualMation();
	V8VM_EXPORT void __stdcall DisposeV8VirtualMation(__int64 vmid);
	V8VM_EXPORT bool __stdcall IsSmartContractLoaded(__int64 vmid, const char* contract_name);
	V8VM_EXPORT bool __stdcall LoadSmartContractBySourcecode(__int64 vmid, const char* contract_name, const char* sourcecode);
	V8VM_EXPORT bool __stdcall LoadSmartContractByFileName(__int64 vmid, const char* contract_name, const char* filename);
	V8VM_EXPORT int __stdcall InvokeSmartContract(__int64 vmid, const char* contract_name, int param1, const char* param2);

#ifdef __cplusplus
}
#endif
