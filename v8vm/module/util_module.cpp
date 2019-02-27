#include "vm_module.h"
#include "virtual_mation.h"
#include "v8vm_internal.h"

inline v8::Local<v8::Private> IndexToPrivateSymbol(V8VirtualMation* vm, uint32_t index)
{
#define V(name, _) &V8VirtualMation::name,
    static v8::Local<v8::Private>(V8VirtualMation::*const methods[])() const =
    {
        PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(V)
    };
#undef V
    CHECK_LT(index, arraysize(methods));
    return (vm->*methods[index])();
}

void InitializeUtilModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    v8::Isolate* isolate = vm->GetIsolate();
    uint32_t index = 0;
#define V(name, _)  \
    exports->Set(context, FIXED_ONE_BYTE_STRING(isolate, #name), v8::Integer::NewFromUnsigned(isolate, index++)).FromJust();
    PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(V)
#undef V
}

NODE_BUILTIN_MODULE_CONTEXT_AWARE(util, InitializeUtilModule)
