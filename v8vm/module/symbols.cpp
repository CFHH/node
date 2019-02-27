#include "vm_module.h"
#include "virtual_mation.h"
#include "v8vm_internal.h"

void InitializeSymbolsModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    v8::Isolate* isolate = vm->GetIsolate();
#define V(PropertyName, StringValue)    \
    exports->Set(context, vm->PropertyName()->Name(), vm->PropertyName()).FromJust();
    PER_ISOLATE_SYMBOL_PROPERTIES(V)
#undef V
}

NODE_MODULE_CONTEXT_AWARE_INTERNAL(symbols, InitializeSymbolsModule)
