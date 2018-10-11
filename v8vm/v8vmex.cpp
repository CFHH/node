#include "v8vm.h"
#include "v8vmex.h"
#include "util.h"

BalanceTransfer_callback BalanceTransfer = NULL;

V8VM_EXTERN void __stdcall SetBalanceTransfer(BalanceTransfer_callback fn)
{
    BalanceTransfer = fn;
}

void BalanceTransfer_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 2)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    int value1 = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    v8::String::Utf8Value value2(args.GetIsolate(), args[1]);
    int result = BalanceTransfer(value1, *value2);
    args.GetReturnValue().Set(result);
}
