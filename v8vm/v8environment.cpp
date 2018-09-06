#include "v8environment.h"
#include "virtual_mation.h"
#include "smart_contract.h"

V8Environment::V8Environment()
    : m_next_vmid(0)
{
    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
    m_platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(m_platform.get());
    v8::V8::Initialize();
}

V8Environment::~V8Environment()
{
    for (std::map<__int64, V8VirtualMation*>::iterator itr = m_vms.begin(); itr != m_vms.end(); ++itr)
    {
        V8VirtualMation* vm = itr->second;
        delete vm;
    }
    m_vms.clear();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
}

void V8Environment::PumpMessage(v8::Isolate* isolate)
{
    v8::Platform* platform = m_platform.get();
    while (v8::platform::PumpMessageLoop(platform, isolate))
        continue;
}

V8VirtualMation* V8Environment::CreateVirtualMation()
{
    ++m_next_vmid;
    V8VirtualMation* vm = new V8VirtualMation(this, m_next_vmid);
    m_vms[m_next_vmid] = vm;
    return vm;
}

void V8Environment::DisposeVirtualMation(__int64 vmid)
{
    std::map<__int64, V8VirtualMation*>::iterator itr = m_vms.find(vmid);
    if (itr == m_vms.end())
        return;
    V8VirtualMation* vm = itr->second;
    m_vms.erase(itr);
    delete vm;
}

V8VirtualMation* V8Environment::GetVirtualMation(__int64 vmid)
{
    std::map<__int64, V8VirtualMation*>::iterator itr = m_vms.find(vmid);
    if (itr == m_vms.end())
        return NULL;
    V8VirtualMation* vm = itr->second;
    return vm;
}
