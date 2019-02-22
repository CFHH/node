#include "v8.h"
#include "module.h"

static void GetName(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    //Environment* env = Environment::GetCurrent(args);
    //const char* rval = "Zhangzw";
    //args.GetReturnValue().Set(OneByteString(env->isolate(), rval));
}


void InitializeTestModule(v8::Local<v8::Object> target, v8::Local<v8::Value> unused, v8::Local<v8::Context> context)
{
    //Environment* env = Environment::GetCurrent(context);
    //env->SetMethod(target, "getname", GetName);
}

NODE_BUILTIN_MODULE_CONTEXT_AWARE(testmod, InitializeTestModule)
