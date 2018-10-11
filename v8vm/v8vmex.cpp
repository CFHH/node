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
    int vmid = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    v8::String::Utf8Value from(args.GetIsolate(), args[1]);
    v8::String::Utf8Value to(args.GetIsolate(), args[2]);
    int amount = args[3]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    //args[3]->ToBigInt(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    int result = BalanceTransfer(vmid, *from, *to, amount);
    args.GetReturnValue().Set(result);
}
