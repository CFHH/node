#include "v8vm.h"
#include "v8environment.h"
#include "virtual_mation.h"
#include "smart_contract.h"
#include "util.h"

/*
参考 https://v8.dev/docs/build
假设工作目录是/PATH
一、安装depot_tools
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
        写入环境变量~/.bashr，export PATH=$PATH:/PATH/depot_tools
    gclient
二、得v8代码
    mkdir /PATH/v8
    cd /PATH/v8
    fetch v8
    cd /PATH/v8/v8
    gclient sync
    ./build/install-build-deps.sh
三、编译
    cd /PATH/v8/v8
    git pull && gclient sync
    tools/dev/gm.py x64.release 或 tools/dev/gm.py x64.release.check(编完跑测试用例)
四、用GN编译成动态库
    参考 https://v8.dev/docs/build-gn
    参考 https://blog.scaleprocess.net/building-v8-on-arch-linux/
    1、创建out.gn目录，导入默认的配置x64.release
        tools/dev/v8gen.py x64.release
    2、编辑配置
        gn args out.gn/x64.release
        如果要编译动态库，添加
            is_component_build = true
    3、编译
        ninja -C out.gn/x64.release
        如果要编译静态库（并不是这样）
            ninja -C out.gn/x64.release v8
    4、编译产物
        out.gn/x64.release/*.so
*/

V8Environment* g_environment = NULL;

V8VM_EXTERN void V8VM_STDCALL InitializeV8Environment()
{
    if (g_environment != NULL)
        return;
    g_environment = new V8Environment();
}

V8VM_EXTERN void V8VM_STDCALL ShutdownV8Environment()
{
    if (g_environment == NULL)
        return;
    delete g_environment;
    g_environment = NULL;
}

V8VM_EXTERN Int64 V8VM_STDCALL CreateV8VirtualMation(Int64 expected_vmid)
{
    if (g_environment == NULL)
        return 0;
    V8VirtualMation* vm = g_environment->CreateVirtualMation(expected_vmid);
    if (vm == NULL)
        return 0;
    return vm->VMID();
}

V8VM_EXTERN void V8VM_STDCALL DisposeV8VirtualMation(Int64 vmid)
{
    if (g_environment == NULL)
        return;
    g_environment->DisposeVirtualMation(vmid);
}

V8VM_EXTERN bool V8VM_STDCALL IsV8VirtualMationInUse(Int64 vmid)
{
    if (g_environment == NULL)
        return false;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return false;
    return vm->IsInUse();
}

V8VM_EXTERN bool V8VM_STDCALL IsSmartContractLoaded(Int64 vmid, const char* contract_name)
{
    if (g_environment == NULL)
        return false;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return false;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract == NULL)
        return false;
    return true;
}

V8VM_EXTERN bool V8VM_STDCALL LoadSmartContractBySourcecode(Int64 vmid, const char* contract_name, const char* sourcecode)
{
    if (g_environment == NULL)
        return false;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return false;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract != NULL)
        return false;
    contract = vm->CreateSmartContract(contract_name, sourcecode);
    if (contract == NULL)
        return false;
    return true;
}

V8VM_EXTERN bool V8VM_STDCALL LoadSmartContractByFileName(Int64 vmid, const char* contract_name, const char* filename)
{
    char* buf = NULL;
    bool result = ReadScriptFile(filename, buf);
    if (!result)
    {
        delete buf;
        return false;
    }
    result = LoadSmartContractBySourcecode(vmid, contract_name, buf);
    delete buf;
    return result;
}

V8VM_EXTERN int V8VM_STDCALL InvokeSmartContract(Int64 vmid, const char* contract_name, int param1, const char* param2)
{
    if (g_environment == NULL)
        return -1;
    V8VirtualMation* vm = g_environment->GetVirtualMation(vmid);
    if (vm == NULL)
        return -1;
    SmartContract* contract = vm->GetSmartContract(contract_name);
    if (contract == NULL)
        return -1;

    InvokeParam param;
    param.param0 = int(vmid); //ZZWTODO
    param.param1 = param1;
    param.param2 = param2;
    bool result = contract->Invoke(&param);
    if (!result)
        return -1;
    return 0;
}
