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
GetIntValue_callback GetIntValueFn = NULL;
GetStringValue_callback GetStringValueFn = NULL;

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

V8VM_EXTERN void V8VM_STDCALL SetValueGetter(GetIntValue_callback fn1, GetStringValue_callback fn2)
{
    GetIntValueFn = fn1;
    GetStringValueFn = fn2;
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
    Int64 amount = args[2]->IntegerValue(context).FromMaybe(0);  //ZZWTODO js无法提供int64
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

void GetIntValue_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    Int32 keyid = args[0]->Int32Value(context).FromMaybe(0);
    int result = GetIntValueFn(vmid, keyid);
    args.GetReturnValue().Set(result);
}

void GetStringValue_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() != 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(context);
    Int64 vmid = vm->VMID();
    Int32 keyid = args[0]->Int32Value(context).FromMaybe(0);
    Int64 ptr = GetStringValueFn(vmid, keyid);
    if (ptr != 0)
    {
        char* value_json = (char*)ptr;
        v8::Local<v8::String> result = v8::String::NewFromUtf8(isolate, value_json, v8::NewStringType::kNormal).ToLocalChecked();
        args.GetReturnValue().Set(result);
        DeleteString(ptr);
    }
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



/************************************************************************
AsmJsParser
AsmJsScanner

V8 Bytecode：
    所有指令的列表 E:\Github\node\deps\v8\src\interpreter\bytecodes.h
    class Bytecode

    搜索 BYTECODE_LIST，最主要的就是下面几个
        DECLARE_VISIT_BYTECODE(name, ...)           Visit##name()
                                                    VisitLdaSmi
        DEFINE_BYTECODE_OUTPUT(name, ...)           Create##name##Node()    Output##name()
                                                    OutputLdaSmi
        DEFINE_BYTECODE_NODE_CREATOR(name, ...)     BytecodeNode Name()





    以LdaSmi指令为例，枚举值 kLdaSmi


AST


INTERPRETER
    IGNITION_HANDLER(LdaSmi, InterpreterAssembler)
        声明一个类 class LdaSmiAssembler : public InterpreterAssembler
        static void LdaSmiAssembler::Generate() 会调用 GenerateImpl()
        每个具体的 IGNITION_HANDLER 后面的代码是 void LdaSmiAssembler::GenerateImpl()的实现
        除了IGNITION_HANDLER，还有 TF_STUB TF_BUILTIN 不知道是做什么的

    栈
    Isolate::Init
        SetupIsolateDelegate::SetupInterpreter
            InstallBytecodeHandlers
                SetupInterpreter::InstallBytecodeHandler
                    GenerateBytecodeHandler
                        Name##Assembler::Generate

    builtins-utils-gen.h
************************************************************************/
