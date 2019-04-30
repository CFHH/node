#pragma once
#include "v8.h"

class V8VirtualMation;

/****************************************************************************************************
* 定义Persistent，析构时能自动释放V8资源
* 在V8VirtualMation的ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES里用到
****************************************************************************************************/
template <typename T>
struct ResetInDestructorPersistentTraits
{
    //析构时~Persistent()，调用Reset()
    static const bool kResetInDestructor = true;
    //不允许拷贝，只声明不实现
    template <typename S, typename M>
    inline static void Copy(const v8::Persistent<S, M>&, v8::Persistent<T, ResetInDestructorPersistentTraits<T>>*);
};

template <typename T>
using Persistent = v8::Persistent<T, ResetInDestructorPersistentTraits<T>>;

template <class TypeName>
inline v8::Local<TypeName> PersistentToLocal(v8::Isolate* isolate, const Persistent<TypeName>& persistent)
{
    if (persistent.IsWeak())
        return WeakPersistentToLocal(isolate, persistent);
    else
        return StrongPersistentToLocal(persistent);
}

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentToLocal(const Persistent<TypeName>& persistent)
{
    return *reinterpret_cast<v8::Local<TypeName>*>(const_cast<Persistent<TypeName>*>(&persistent));
}

template <class TypeName>
inline v8::Local<TypeName> WeakPersistentToLocal(v8::Isolate* isolate, const Persistent<TypeName>& persistent)
{
    return v8::Local<TypeName>::New(isolate, persistent);
}


/****************************************************************************************************
* 创建销毁Isolate和Context，需要做些统一的事情，因此单独提出
****************************************************************************************************/
v8::Isolate* NewIsolate(v8::Isolate::CreateParams* params);
void OnDisposeIsolate(v8::Isolate* isolate);
v8::Local<v8::Context> NewContext(v8::Isolate* isolate, V8VirtualMation* vm, v8::Local<v8::ObjectTemplate> object_template = v8::Local<v8::ObjectTemplate>());
void OnDisposeContext(v8::Isolate* isolate, v8::Local<v8::Context> context);


/****************************************************************************************************
* 加载脚本
****************************************************************************************************/
v8::MaybeLocal<v8::Value> LoadScript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* script_name);


/****************************************************************************************************
* 其他
****************************************************************************************************/

#define FIXED_ONE_BYTE_STRING(isolate, str) OneByteString(isolate, str, sizeof(str) - 1)  //ZZWTODO 为什么-1？

inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate, const char* data, int length = -1)
{
    return v8::String::NewFromOneByte(isolate, reinterpret_cast<const uint8_t*>(data), v8::NewStringType::kNormal, length).ToLocalChecked();
}

template <typename T, size_t N>
constexpr size_t arraysize(const T(&)[N]) { return N; }

enum ErrorHandlingMode
{
    CONTEXTIFY_ERROR,
    FATAL_ERROR,
    MODULE_ERROR
};

#ifndef V8VM_CONTEXT_EMBEDDER_DATA_INDEX
#define V8VM_CONTEXT_EMBEDDER_DATA_INDEX 32
#endif
#ifndef V8VM_CONTEXT_SANDBOX_OBJECT_INDEX
#define V8VM_CONTEXT_SANDBOX_OBJECT_INDEX 33
#endif
#ifndef V8VM_CONTEXT_ALLOW_WASM_CODE_GENERATION_INDEX
#define V8VM_CONTEXT_ALLOW_WASM_CODE_GENERATION_INDEX 34
#endif

enum ContextEmbedderIndex
{
    kEnvironment = V8VM_CONTEXT_EMBEDDER_DATA_INDEX,
    kSandboxObject = V8VM_CONTEXT_SANDBOX_OBJECT_INDEX,
    kAllowWasmCodeGeneration = V8VM_CONTEXT_ALLOW_WASM_CODE_GENERATION_INDEX,
};
