package v8vm

/*
#cgo CFLAGS: -Iinclude
#include <stdlib.h>
*/
import "C"
import "fmt"

//export BalanceTransfer
func BalanceTransfer(vmid int64, from *C.char, to *C.char, amount int64) _Ctype_int {
	fmt.Println("!!!!!!!!!! BalanceTransfer(), vmid =", vmid, ", from =", C.GoString(from), ", to =", C.GoString(to), ", value2 =", amount)
	return 0
}
