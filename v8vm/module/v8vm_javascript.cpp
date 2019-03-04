#include "v8vm_javascript.h"
#include "virtual_mation.h"

void DefineJavaScript(V8VirtualMation* vm, v8::Local<v8::Object> exports)
{
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();
}
