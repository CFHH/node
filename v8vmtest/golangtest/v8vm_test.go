package v8vm

import (
	"fmt"
	"testing"
)

func TestV8VM(t *testing.T) {
	fmt.Println("'TransactionCallback(234, \"CallFromJS\")'")

	ret := DllAdd()
	fmt.Println("DllAdd return:", ret)
	// rcstr := ExecuteTX("'Hello Go World'")
	// fmt.Println("ExecuteTX ret:", rcstr)
	result := ExecuteTX2()
	fmt.Println("ExecuteTX2 return:", result)
}
