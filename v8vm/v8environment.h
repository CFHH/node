#pragma once

#include <map>
#include "libplatform/libplatform.h"
#include "v8.h"
#include "platform.h"

class V8VirtualMation;

class V8Environment
{
public:
    explicit V8Environment();
    ~V8Environment();
    v8::Platform* GetPlatform() { return m_platform.get(); }
    void PumpMessage(v8::Isolate* isolate);
    V8VirtualMation* CreateVirtualMation(Int64 expected_vmid);
    void DisposeVirtualMation(Int64 vmid);
    V8VirtualMation* GetVirtualMation(Int64 vmid);

private:
    std::unique_ptr<v8::Platform> m_platform;
    Int64 m_next_vmid;
    std::map<Int64, V8VirtualMation*> m_vms;
};
