#include "vm_module.h"
#include "virtual_mation.h"
#include "v8vm_internal.h"

void InitializeContextifyModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    v8::Isolate* isolate = vm->GetIsolate();
}

NODE_MODULE_CONTEXT_AWARE_INTERNAL(contextify, InitializeContextifyModule)
