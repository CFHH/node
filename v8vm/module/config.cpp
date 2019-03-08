#include "vm_module.h"
#include "virtual_mation.h"
#include "v8vm_internal.h"

bool config_expose_internals = false;
bool config_experimental_worker = false;

#define READONLY_BOOLEAN_PROPERTY(str)  \
    exports->DefineOwnProperty(context, FIXED_ONE_BYTE_STRING(isolate, str), v8::True(isolate), v8::ReadOnly).FromJust();

#define READONLY_PROPERTY(obj, name, value) \
    obj->DefineOwnProperty(context, FIXED_ONE_BYTE_STRING(isolate, name), value, v8::ReadOnly).FromJust();

void InitializeConfigModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    v8::Isolate* isolate = vm->GetIsolate();

    if (config_expose_internals)
        READONLY_BOOLEAN_PROPERTY("exposeInternals");

    if (config_experimental_worker)
        READONLY_BOOLEAN_PROPERTY("experimentalWorker");
}

NODE_MODULE_CONTEXT_AWARE_INTERNAL(config, InitializeConfigModule)
