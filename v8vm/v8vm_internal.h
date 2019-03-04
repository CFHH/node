#pragma once
#include "v8.h"

v8::Isolate* NewIsolate(v8::Isolate::CreateParams* params);
void OnDisposeIsolate(v8::Isolate* isolate);
v8::Local<v8::Context> NewContext(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> object_template = v8::Local<v8::ObjectTemplate>());
void OnDisposeContext(v8::Isolate* isolate, v8::Local<v8::Context> context);
v8::MaybeLocal<v8::Value> LoadScript(v8::Isolate* isolate, v8::Local<v8::Context> context, const char* script_name);



#define FIXED_ONE_BYTE_STRING(isolate, str) OneByteString(isolate, str, sizeof(str) - 1)
inline v8::Local<v8::String> OneByteString(v8::Isolate* isolate, const char* data, int length = -1)
{
    return v8::String::NewFromOneByte(isolate, reinterpret_cast<const uint8_t*>(data), v8::NewStringType::kNormal, length).ToLocalChecked();
}


template <typename T>
struct ResetInDestructorPersistentTraits
{
    static const bool kResetInDestructor = true;

    // Disallow copy semantics by leaving this unimplemented.
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


template <typename T, size_t N>
constexpr size_t arraysize(const T(&)[N]) { return N; }

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
