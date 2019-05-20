#include <stdlib.h>
#include <stdio.h>
#include "v8vm.h"
#include "v8vm_ex.h"
#include "v8vm_internal.h"
#include "v8vm_util.h"
#include "virtual_mation.h"

extern const char* g_internal_js_lib_path;
extern const char* g_js_source_path;

//这里的变量名不能和go中的c函数部分同名，否则在linux下会发生segmentation violation
Log_callback LogFn = NULL;
BalanceTransfer_callback BalanceTransferFn = NULL;
UpdateDB_callback UpdateDBFn = NULL;
QueryDB_callback QueryDBFn = NULL;
RequireAuth_callback RequireAuthFn = NULL;

V8VM_EXTERN void V8VM_STDCALL SetLog(Log_callback fn)
{
    LogFn = fn;
}

V8VM_EXTERN void V8VM_STDCALL SetBalanceTransfer(BalanceTransfer_callback fn)
{
    BalanceTransferFn = fn;
}

V8VM_EXTERN void V8VM_STDCALL SetUpdateDB(UpdateDB_callback fn)
{
    UpdateDBFn = fn;
}

V8VM_EXTERN void V8VM_STDCALL SetQueryDB(QueryDB_callback fn)
{
    QueryDBFn = fn;
}

V8VM_EXTERN void V8VM_STDCALL SetRequireAuth(RequireAuth_callback fn)
{
    RequireAuthFn = fn;
}

void BalanceTransfer_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 3)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    v8::String::Utf8Value from(isolate, args[0]);
    v8::String::Utf8Value to(isolate, args[1]);
    Int64 amount = args[2]->IntegerValue(context).FromMaybe(0);
    int result = BalanceTransferFn(vmid, *from, *to, amount);
    args.GetReturnValue().Set(result);
}

void UpdateDB_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 2)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    v8::String::Utf8Value key_json(isolate, args[0]);
    v8::String::Utf8Value value_json(isolate, args[1]);
    int result = UpdateDBFn(vmid, *key_json, *value_json);
    args.GetReturnValue().Set(result);
}

void QueryDB_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    v8::String::Utf8Value key_json(isolate, args[0]);
    Int64 ptr = QueryDBFn(vmid, *key_json);
    if (ptr != 0)
    {
        char* value_json = (char*)ptr;
        v8::Local<v8::String> result = v8::String::NewFromUtf8(isolate, value_json, v8::NewStringType::kNormal).ToLocalChecked();
        args.GetReturnValue().Set(result);
        DeleteString(ptr);
    }
}

void RequireAuth_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    v8::String::Utf8Value account(isolate, args[0]);
    int result = RequireAuthFn(vmid, *account);
    args.GetReturnValue().Set(result);
}

void LoadScript_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::String::Utf8Value file_name(isolate, args[0]);
    v8::MaybeLocal<v8::Value> result = LoadSourceScript(isolate, context, *file_name, true);
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
    v8::String::Utf8Value relative_file_name(isolate, args[0]);

    std::string filename(g_js_source_path);
    filename += *relative_file_name;
    bool isok = false;
    FILE* filehandle = fopen(filename.c_str(), "r");
    if (filehandle != NULL)
    {
        isok = true;
        fclose(filehandle);
    }
    args.GetReturnValue().Set(isok);
}
