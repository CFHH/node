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
    if (args.Length() < 3)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::String::Utf8Value from(args.GetIsolate(), args[0]);
    v8::String::Utf8Value to(args.GetIsolate(), args[1]);
    int amount = args[2]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    //args[2]->ToBigInt(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    int result = BalanceTransfer(*from, *to, amount);
    args.GetReturnValue().Set(result);
}
