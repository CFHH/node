#include "vm_module.h"

static void GetName(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    //Environment* env = Environment::GetCurrent(args);
    //const char* rval = "Zhangzw";
    //args.GetReturnValue().Set(OneByteString(env->isolate(), rval));
}


void InitializeTestModule(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data)
{
    //Environment* env = Environment::GetCurrent(context);
    //env->SetMethod(target, "getname", GetName);
}

NODE_BUILTIN_MODULE_CONTEXT_AWARE(testmod, InitializeTestModule)
