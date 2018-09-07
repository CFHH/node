#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "v8vm.h"


int main(int argc, char* argv[])
{
    InitializeV8Environment();
    __int64 vmid1 = CreateV8VirtualMation();
    if (vmid1 == 0)
        return 1;
    __int64 vmid2 = CreateV8VirtualMation();
    if (vmid1 == 0)
        return 1;
    bool ok1 = LoadSmartContractByFileName(vmid1, "sample_contract_1", "E:/Github/node/v8vmtest/sample_contract.js");
    if (!ok1)
        return 1;
    bool ok2 = LoadSmartContractByFileName(vmid2, "sample_contract_2", "E:/Github/node/v8vmtest/sample_contract.js");
    if (!ok2)
        return 1;
    int result1 = InvokeSmartContract(vmid1, "sample_contract_1", 1, "1111111111");
    if (result1 != 0)
        return 1;
    int result2 = InvokeSmartContract(vmid2, "sample_contract_2", 1, "2222222222");
    if (result2 != 0)
        return 1;
    DisposeV8VirtualMation(vmid1);
    DisposeV8VirtualMation(vmid2);
    ShutdownV8Environment();
	return 0;
}
