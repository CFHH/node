#pragma once

#include <map>
#include "libplatform/libplatform.h"
#include "v8.h"

class V8VirtualMation;

class V8Environment
{
public:
    explicit V8Environment();
    ~V8Environment();
    v8::Platform* GetPlatform() {  return m_platform.get(); }
    void PumpMessage(v8::Isolate* isolate);
    V8VirtualMation* CreateVirtualMation();
    void DisposeVirtualMation(__int64 vmid);
    V8VirtualMation* GetVirtualMation(__int64 vmid);

private:
    std::unique_ptr<v8::Platform> m_platform;
    __int64 m_next_vmid;
    std::map<__int64, V8VirtualMation*> m_vms;
};
