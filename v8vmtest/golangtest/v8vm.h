#pragma once

#ifdef V8VM_EXPORTS
#define V8VM_EXPORT  __declspec(dllexport)
#else
#define V8VM_EXPORT  __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	V8VM_EXPORT void __stdcall InitializeVM();
	V8VM_EXPORT void __stdcall DisposeVM();
	V8VM_EXPORT int __stdcall ExecuteTransaction(const char *jssrc);
	V8VM_EXPORT int __stdcall AddFunc(int a, int b);


	typedef int(*txansaction_callback_fcn)(int, char*);
	V8VM_EXPORT void __stdcall SetTransactionCallback(txansaction_callback_fcn);

	typedef int(*txansaction_result)(int, char*);
	V8VM_EXPORT void __stdcall SetTransactionResult(txansaction_result);

	typedef void(*txansaction_result_void)(int, char*);
	V8VM_EXPORT void __stdcall SetTransactionResultVoid(txansaction_result_void);

#ifdef __cplusplus
}
#endif
