#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "v8vm.h"

#define SET_INTERNAL_JS_LIB_PATH SetInternalJSLibPath("E:/Github/node/v8vm/jslib")
#define SET_JS_SOURCE_PATH SetJSSourcePath("E:/Github/node/v8vmtest")

const char* SampleContractName = "SampleContract";
const char* SampleContract = "function Process(param)\
{\
    if (!output[param.param2])\
    {\
        output[param.param2] = param.param1;\
    }\
    else\
    {\
        output[param.param2] = Number(output[param.param2]) + param.param1\
    }\
    log(\"Process(), \" + param.param2 + \" = \" + output[param.param2]);\
}\
\
exports.Process = Process;";

int TestCase1()
{
    SET_INTERNAL_JS_LIB_PATH;
    SET_JS_SOURCE_PATH;
    InitializeV8Environment();
    Int64 vmid = CreateV8VirtualMation(0);
    if (vmid == 0)
        return 1;
    bool ok1 = LoadSmartContractBySourcecode(vmid, SampleContractName, SampleContract);
    if (!ok1)
        return 1;
    for (int i = 0; i < 4; ++i)
    {
        int result1 = InvokeSmartContract(vmid, SampleContractName, i + 1, "sum");
        if (result1 != 0)
            return 1;
    }
    DisposeV8VirtualMation(vmid);
    ShutdownV8Environment();
    return 0;
}

int TestCase2()
{
    SET_INTERNAL_JS_LIB_PATH;
    SET_JS_SOURCE_PATH;
    InitializeV8Environment();
    Int64 vmid = CreateV8VirtualMation(0);
    if (vmid == 0)
        return 1;
    bool ok1 = LoadSmartContractByFileName(vmid, SampleContractName, "sample_contract.js");
    if (!ok1)
        return 1;
    for (int i = 0; i < 4; ++i)
    {
        int result1 = InvokeSmartContract(vmid, SampleContractName, i + 1, "sum");
        if (result1 != 0)
            return 1;
    }
    DisposeV8VirtualMation(vmid);
    ShutdownV8Environment();
    return 0;
}

int TestCase3()
{
    SET_INTERNAL_JS_LIB_PATH;
    SET_JS_SOURCE_PATH;
    InitializeV8Environment();
    Int64 vmid1 = CreateV8VirtualMation(0);
    if (vmid1 == 0)
        return 1;
    Int64 vmid2 = CreateV8VirtualMation(0);
    if (vmid2 == 0)
        return 1;
    bool ok1 = LoadSmartContractBySourcecode(vmid1, "sample_contract_1", SampleContract);
    if (!ok1)
        return 1;
    bool ok2 = LoadSmartContractBySourcecode(vmid2, "sample_contract_2", SampleContract);
    if (!ok2)
        return 1;
    int result1 = InvokeSmartContract(vmid1, "sample_contract_1", 100, "vm1");
    if (result1 != 0)
        return 1;
    int result2 = InvokeSmartContract(vmid2, "sample_contract_2", 200, "vm2");
    if (result2 != 0)
        return 1;
    DisposeV8VirtualMation(vmid1);
    DisposeV8VirtualMation(vmid2);
    ShutdownV8Environment();
    return 0;
}

#ifdef _MSC_VER
#include <Windows.h>
#else
#endif
int TestCase4()
{
    SET_INTERNAL_JS_LIB_PATH;
    SET_JS_SOURCE_PATH;
    InitializeV8Environment();
    Int64 vmid = CreateV8VirtualMation(0);
    if (vmid == 0)
        return 1;
    bool ok1 = LoadSmartContractBySourcecode(vmid, SampleContractName, SampleContract);
    if (!ok1)
        return 1;
#ifdef _MSC_VER
    LARGE_INTEGER freq;
    LARGE_INTEGER start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
#else
#endif
    for (int i = 0; i < 10000; ++i)
    {
        int result1 = InvokeSmartContract(vmid, SampleContractName, i + 1, "sum");
        if (result1 != 0)
            return 1;
    }
#ifdef _MSC_VER
    QueryPerformanceCounter(&end);
    double cost = double(end.QuadPart - start.QuadPart) / double(freq.QuadPart);
    printf("%f\n", cost);
#else
#endif
    DisposeV8VirtualMation(vmid);
    ShutdownV8Environment();
    return 0;
}

const char* BalancetransferContractName = "BalancetransferContract";
extern "C"
{
    int BalanceTransferCallback(Int64 vmid, char* from, char* to, Int64 amount)
    {
        printf("BalanceTransferCallback, FROM : {%s}, TO :{%s}, AMOUNT : {%i}\r\n", from, to, int(amount));
        return 0;
    }
}
int TestCase5()
{
    SET_INTERNAL_JS_LIB_PATH;
    SET_JS_SOURCE_PATH;
    SetBalanceTransfer(BalanceTransferCallback);
    InitializeV8Environment();
    Int64 vmid = CreateV8VirtualMation(0);
    if (vmid == 0)
        return 1;
    bool ok1 = LoadSmartContractByFileName(vmid, BalancetransferContractName, "/block_balancetransfer_contract.js");
    if (!ok1)
        return 1;
    for (int i = 0; i < 4; ++i)
    {
        int result1 = InvokeSmartContract(vmid, BalancetransferContractName, 0, "{ \"from\":\"Zhang3\", \"to\":\"Li4\", \"amount\":100123456789 }");
        if (result1 != 0)
            return 1;
    }
    DisposeV8VirtualMation(vmid);
    ShutdownV8Environment();
    return 0;
}

int main()
{
    TestCase5();
    //char ch;
    //ch = getch();
    return 0;
}

