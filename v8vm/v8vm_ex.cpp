#include <stdio.h>
#include "v8vm.h"
#include "v8vm_ex.h"
#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "virtual_mation.h"

extern const char* g_js_source_path;
//这里的变量名不能和go中的c函数部分同名，否则在linux下会发生segmentation violation
BalanceTransfer_callback BalanceTransferFn = NULL;

V8VM_EXTERN void V8VM_STDCALL SetBalanceTransfer(BalanceTransfer_callback fn)
{
    BalanceTransferFn = fn;
}

void BalanceTransfer_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 4)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    Int64 vmid = args[0]->IntegerValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    v8::String::Utf8Value from(args.GetIsolate(), args[1]);
    v8::String::Utf8Value to(args.GetIsolate(), args[2]);
    Int64 amount = args[3]->IntegerValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    int result = BalanceTransferFn(vmid, *from, *to, amount);
    args.GetReturnValue().Set(result);
}

void LoadScript_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(isolate);
    v8::Local<v8::Context> context = vm->context();
    v8::String::Utf8Value script_name(args.GetIsolate(), args[0]);
    v8::MaybeLocal<v8::Value> result = LoadSourceScript(isolate, context, *script_name, true);
    v8::Local<v8::Value> module;
    if (result.ToLocal(&module))
    {
        args.GetReturnValue().Set(module);
    }
}

void GetSourcePath_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::String> path = v8::String::NewFromUtf8(isolate, g_js_source_path, v8::NewStringType::kNormal).ToLocalChecked();
    args.GetReturnValue().Set(path);
}

void IsFileExists_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::String::Utf8Value file_name(args.GetIsolate(), args[0]);
    FILE* filehandle = fopen(*file_name, "r");
    bool isok = filehandle != NULL;
    fclose(filehandle);
    args.GetReturnValue().Set(isok);
}
