#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "v8vm.h"


//int main(int argc, char* argv[])
//{
//    InitializeV8Environment();
//    __int64 vmid = CreateV8VirtualMation();
//    if (vmid == 0)
//        return 1;
//    bool ok1 = LoadSmartContractByFileName(vmid, "sample_contract_1", "E:/Github/node/v8vmtest/sample_contract.js");
//    if (!ok1)
//        return 1;
//    for (int i = 0; i < 4; ++i)
//    {
//        int result1 = InvokeSmartContract(vmid, "sample_contract_1", i+1, "sum");
//        if (result1 != 0)
//            return 1;
//    }
//    DisposeV8VirtualMation(vmid);
//    ShutdownV8Environment();
//	return 0;
//}

//int main(int argc, char* argv[])
//{
//    InitializeV8Environment();
//    __int64 vmid1 = CreateV8VirtualMation();
//    if (vmid1 == 0)
//        return 1;
//    __int64 vmid2 = CreateV8VirtualMation();
//    if (vmid2 == 0)
//        return 1;
//    bool ok1 = LoadSmartContractByFileName(vmid1, "sample_contract_1", "E:/Github/node/v8vmtest/sample_contract.js");
//    if (!ok1)
//        return 1;
//    bool ok2 = LoadSmartContractByFileName(vmid2, "sample_contract_2", "E:/Github/node/v8vmtest/sample_contract.js");
//    if (!ok2)
//        return 1;
//    int result1 = InvokeSmartContract(vmid1, "sample_contract_1", 100, "vm1");
//    if (result1 != 0)
//        return 1;
//    int result2 = InvokeSmartContract(vmid2, "sample_contract_2", 200, "vm2");
//    if (result2 != 0)
//        return 1;
//    DisposeV8VirtualMation(vmid1);
//    DisposeV8VirtualMation(vmid2);
//    ShutdownV8Environment();
//    return 0;
//}

#include <Windows.h>
int main(int argc, char* argv[])
{
    InitializeV8Environment();
    __int64 vmid = CreateV8VirtualMation();
    if (vmid == 0)
        return 1;
    bool ok1 = LoadSmartContractByFileName(vmid, "sample_contract_1", "E:/Github/node/v8vmtest/sample_contract.js");
    if (!ok1)
        return 1;

    LARGE_INTEGER freq;
    LARGE_INTEGER start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    for (int i = 0; i < 10000; ++i)
    {
        int result1 = InvokeSmartContract(vmid, "sample_contract_1", i + 1, "sum");
        if (result1 != 0)
            return 1;
    }
    QueryPerformanceCounter(&end);
    double cost = double(end.QuadPart - start.QuadPart) / double(freq.QuadPart);
    printf("%f\n", cost);

    DisposeV8VirtualMation(vmid);
    ShutdownV8Environment();
    return 0;
}
