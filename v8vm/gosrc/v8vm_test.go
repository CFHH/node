package v8vm

import (
	"fmt"
	"reflect"
	"testing"
	"time"
	"unsafe"
)

var sample_contract = `
function Initialize()
{
}

function Process(param)
{
	if (!output[param.param2])
	{
		output[param.param2] = param.param1;
	}
	else
	{
		output[param.param2] = Number(output[param.param2]) + param.param1
	}
	//log("Process(), " + param.param2 + " = " + output[param.param2]);
}

Initialize();
`

func TestV8vmCase1(t *testing.T) {
	InitializeV8Environment()
	vmid := CreateV8VirtualMation()
	if vmid == 0 {
		panic("")
	}
	//ok := LoadSmartContractByFileName(vmid, "sample_contract", "sample_contract.js")
	ok := LoadSmartContractBySourcecode(vmid, "sample_contract", sample_contract)
	if !ok {
		panic("")
	}
	for i := 0; i < 4; i++ {
		result := InvokeSmartContract(vmid, "sample_contract", i+1, "sum")
		if result != 0 {
			panic("")
		}
	}
	DisposeV8VirtualMation(vmid)
	ShutdownV8Environment()
}

func TestV8vmCase2(t *testing.T) {
	InitializeV8Environment()
	vmid1 := CreateV8VirtualMation()
	if vmid1 == 0 {
		panic("")
	}
	vmid2 := CreateV8VirtualMation()
	if vmid2 == 0 {
		panic("")
	}
	ok1 := LoadSmartContractBySourcecode(vmid1, "sample_contract", sample_contract)
	if !ok1 {
		panic("")
	}
	ok2 := LoadSmartContractBySourcecode(vmid2, "sample_contract", sample_contract)
	if !ok2 {
		panic("")
	}
	result1 := InvokeSmartContract(vmid1, "sample_contract", 100, "vm1")
	if result1 != 0 {
		panic("")
	}
	result2 := InvokeSmartContract(vmid2, "sample_contract", 200, "vm2")
	if result2 != 0 {
		panic("")
	}
	DisposeV8VirtualMation(vmid1)
	DisposeV8VirtualMation(vmid2)
	ShutdownV8Environment()
}

func TestV8vmCase3(t *testing.T) {
	InitializeV8Environment()
	vmid := CreateV8VirtualMation()
	if vmid == 0 {
		panic("")
	}
	ok := LoadSmartContractBySourcecode(vmid, "sample_contract", sample_contract)
	if !ok {
		panic("")
	}
	t1 := time.Now()
	for i := 0; i < 10000; i++ {
		result := InvokeSmartContract(vmid, "sample_contract", i+1, "sum")
		if result != 0 {
			panic("")
		}
	}
	t2 := time.Now()
	dt := t2.Sub(t1)
	fmt.Printf("Time Cost = %v\n", dt)
	DisposeV8VirtualMation(vmid)
	ShutdownV8Environment()
}

var balancetransfer_contract = `
function Initialize()
{
}

function Process(param)
{
	var obj = JSON.parse(param.param2);
	BalanceTransfer(obj.from, obj.to, obj.amount)
}

Initialize();
`

func TestV8vmCase4(t *testing.T) {
	InitializeV8Environment()
	vmid := CreateV8VirtualMation()
	if vmid == 0 {
		panic("")
	}
	ok := LoadSmartContractBySourcecode(vmid, "balancetransfer_contract", balancetransfer_contract)
	if !ok {
		panic("")
	}
	result := InvokeSmartContract(vmid, "balancetransfer_contract", 0, `{ "from":"Zhang3", "to":"Li4", "amount":100 }`)
	if result != 0 {
		panic("")
	}
	result = InvokeSmartContract(vmid, "balancetransfer_contract", 0, `{ "from":"Li4", "to":"Zhang3", "amount":110 }`)
	if result != 0 {
		panic("")
	}
	DisposeV8VirtualMation(vmid)
	ShutdownV8Environment()
}
