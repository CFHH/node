package v8vm

/*
#cgo CFLAGS: -Iinclude
#cgo windows LDFLAGS: -L. -lv8vm
#include <stdlib.h>
#include "v8vm.h"

int TransactionCallbackStub(int value1, char* value2);
int TransactionResultStub(int value1, char* value2);
void TransactionResultVoidStub(int value1, char* value2);
*/
import "C"

import (
	"fmt"
	"unsafe"
)

//export TransactionCallback
func TransactionCallback(value1 int32, value2 *C.char) _Ctype_int {
	//defer C.free(unsafe.Pointer(value2))
	fmt.Println("!!!!!!!!!! TransactionCallback(), value1 =", value1, ", value2 =", C.GoString(value2))
	return 0
}

//export TransactionResult
func TransactionResult(value1 int32, value2 *C.char) _Ctype_int {
	//defer C.free(unsafe.Pointer(value2))
	fmt.Println("TransactionResult() !!!!!!!!!!, value1 =", value1, ", value2 =", C.GoString(value2))
	return 12345
}

//export TransactionResultVoid
func TransactionResultVoid(value1 int32, value2 *C.char) {
	//defer C.free(unsafe.Pointer(value2))
	fmt.Println("TransactionResultVoid() !!!!!!!!!!, value1 =", value1, ", value2 =", C.GoString(value2))
}

func ExecuteTX(script string) int32 {
	cstr := C.CString(script)
	defer C.free(unsafe.Pointer(cstr))

	C.InitializeVM()
	defer C.DisposeVM()

	result := C.ExecuteTransaction(cstr)
	return int32(result)
}

func DllAdd() int32 {
	rcstr := C.AddFunc(1, 3)
	return int32(rcstr)
}

func ExecuteTX2() int32 {
	C.SetTransactionCallback((C.txansaction_callback_fcn)(unsafe.Pointer(C.TransactionCallbackStub)))
	C.SetTransactionResult((C.txansaction_result)(unsafe.Pointer(C.TransactionResultStub)))
	//C.SetTransactionResult((C.txansaction_result_void)(unsafe.Pointer(C.TransactionResultVoidStub)))

	C.InitializeVM()

	cstr := C.CString("TransactionCallback(234, \"CallFromJS\")")
	//cstr := C.CString("'111+222'")
	//cstr := C.CString("111+222")
	result := C.ExecuteTransaction(cstr)

	C.DisposeVM()
	C.free(unsafe.Pointer(cstr))
	return int32(result) //_Ctype_int
}
