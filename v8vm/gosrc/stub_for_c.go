package v8vm

/*
#cgo CFLAGS: -Iinclude
#include <stdlib.h>
*/
import "C"
import "fmt"

//export BalanceTransfer
func BalanceTransfer(value1 int32, value2 *C.char) _Ctype_int {
	fmt.Println("!!!!!!!!!! BalanceTransfer(), value1 =", value1, ", value2 =", C.GoString(value2))
	return 0
}
