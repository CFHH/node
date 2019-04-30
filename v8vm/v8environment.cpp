#include "v8environment.h"
#include "virtual_mation.h"
#include "smart_contract.h"
#include "vm_module.h"

bool v8_initialized = false;

V8Environment::V8Environment()
    : m_next_vmid(1)
{
    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
    //RegisterBuiltinModules(); //ZZWTODO COMMONJS NODE
    m_platform = v8::platform::NewDefaultPlatform(); //ZZWTODO 自定义Platform的工作量太大了，参考v8里的DefaultPlatform和node里的NodePlatform
    v8::V8::InitializePlatform(m_platform.get());
    v8::V8::Initialize();
    v8_initialized = true;
}

V8Environment::~V8Environment()
{
    for (std::map<Int64, V8VirtualMation*>::iterator itr = m_vms.begin(); itr != m_vms.end(); ++itr)
    {
        V8VirtualMation* vm = itr->second;
        delete vm;
    }
    m_vms.clear();
    v8_initialized = false;
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    //ZZWTODO 此时可以销毁自定义Platform，NewDefaultPlatform()返回的是智能指针封装的，因此不用处理
}

void V8Environment::PumpMessage(v8::Isolate* isolate)
{
    v8::Platform* platform = m_platform.get();
    while (v8::platform::PumpMessageLoop(platform, isolate))
        continue;
}

V8VirtualMation* V8Environment::CreateVirtualMation(Int64 expected_vmid)
{
    Int64 vmid = expected_vmid;
    if (expected_vmid != 0)
    {
        if (m_vms.find(expected_vmid) != m_vms.end())
            return NULL;
    }
    else
    {
        while (true)
        {
            if (m_vms.find(m_next_vmid) != m_vms.end())
            {
                if (++m_next_vmid == 0)
                    ++m_next_vmid;
            }
            else
            {
                vmid = m_next_vmid;
                if (++m_next_vmid == 0)
                    ++m_next_vmid;
                break;
            }
        }
    }
    V8VirtualMation* vm = new V8VirtualMation(this, vmid);
    m_vms[vmid] = vm;
    return vm;
}

void V8Environment::DisposeVirtualMation(Int64 vmid)
{
    std::map<Int64, V8VirtualMation*>::iterator itr = m_vms.find(vmid);
    if (itr == m_vms.end())
        return;
    V8VirtualMation* vm = itr->second;
    m_vms.erase(itr);
    delete vm;
}

V8VirtualMation* V8Environment::GetVirtualMation(Int64 vmid)
{
    std::map<Int64, V8VirtualMation*>::iterator itr = m_vms.find(vmid);
    if (itr == m_vms.end())
        return NULL;
    V8VirtualMation* vm = itr->second;
    return vm;
}
