package v8vm

/*
#cgo CFLAGS: -Iinclude
#cgo windows LDFLAGS: -L. -lv8vm
#include <stdbool.h> //C90 does not support the boolean data type; C99 does include it with this include
#include <stdlib.h>
#include <string.h>
#include "v8vm.h"

int BalanceTransferStub(__int64 vmid, char* from, char* to, __int64 amount);
*/
import "C"
import "unsafe"

func SetCCallback() {
	C.SetBalanceTransfer((C.BalanceTransfer_callback)(unsafe.Pointer(C.BalanceTransferStub)))
}

func InitializeV8Environment() {
	C.InitializeV8Environment()
	SetCCallback()
}

func ShutdownV8Environment() {
	C.ShutdownV8Environment()
}

func CreateV8VirtualMation(expected_vmid int64) int64 {
	cid := C.longlong(expected_vmid)
	vmid := C.CreateV8VirtualMation(cid)
	return int64(vmid)
}

func DisposeV8VirtualMation(vmid int64) {
	cid := C.longlong(vmid)
	C.DisposeV8VirtualMation(cid)
}

func IsSmartContractLoaded(vmid int64, contract_name string) bool {
	cid := C.longlong(vmid)
	cname := C.CString(contract_name)
	defer C.free(unsafe.Pointer(cname))
	result := C.IsSmartContractLoaded(cid, cname)
	return bool(result)
}

func LoadSmartContractBySourcecode(vmid int64, contract_name string, sourcecode string) bool {
	cid := C.longlong(vmid)
	cname := C.CString(contract_name)
	defer C.free(unsafe.Pointer(cname))
	ccode := C.CString(sourcecode)
	defer C.free(unsafe.Pointer(ccode))
	result := C.LoadSmartContractBySourcecode(cid, cname, ccode)
	return bool(result)
}

func LoadSmartContractByFileName(vmid int64, contract_name string, filename string) bool {
	cid := C.longlong(vmid)
	cname := C.CString(contract_name)
	defer C.free(unsafe.Pointer(cname))
	cfile := C.CString(filename)
	defer C.free(unsafe.Pointer(cfile))
	result := C.LoadSmartContractBySourcecode(cid, cname, cfile)
	return bool(result)
}

func InvokeSmartContract(vmid int64, contract_name string, param1 int, param2 string) int32 {
	cid := C.longlong(vmid)
	cname := C.CString(contract_name)
	defer C.free(unsafe.Pointer(cname))
	cparam1 := C.int(param1)
	cparam2 := C.CString(param2)
	defer C.free(unsafe.Pointer(cparam2))
	result := C.InvokeSmartContract(cid, cname, cparam1, cparam2)
	return int32(result)
}
