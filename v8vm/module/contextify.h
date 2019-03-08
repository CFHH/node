#pragma once
#include "libplatform/libplatform.h"
#include "v8.h"
#include "platform.h"
#include "v8vm_internal.h"
#include "base_object.h"

class V8VirtualMation;

struct ContextOptions
{
    v8::Local<v8::String> name;
    v8::Local<v8::String> origin;
    v8::Local<v8::Boolean> allow_code_gen_strings;
    v8::Local<v8::Boolean> allow_code_gen_wasm;
};

class ContextifyContext
{
public:
    ContextifyContext(V8VirtualMation* vm, v8::Local<v8::Object> sandbox_obj, const ContextOptions& options);

    v8::Local<v8::Value> CreateDataWrapper(V8VirtualMation* vm);
    v8::Local<v8::Context> CreateV8Context(V8VirtualMation* vm, v8::Local<v8::Object> sandbox, const ContextOptions& options);

    V8VirtualMation* vm() const { return m_vm; }
    v8::Local<v8::Context> context() const;
    v8::Local<v8::Object> global_proxy() const { return context()->Global(); }
    v8::Local<v8::Object> sandbox() const { return v8::Local<v8::Object>::Cast(context()->GetEmbedderData(ContextEmbedderIndex::kSandboxObject)); }

    static ContextifyContext* ContextFromContextifiedSandbox(V8VirtualMation* vm, const v8::Local<v8::Object>& sandbox);
    template <typename T>
    static ContextifyContext* Get(const v8::PropertyCallbackInfo<T>& args);

    static void InitializeContextifyModule(V8VirtualMation* vm, v8::Local<v8::Object> target);

private:
    static void MakeContext(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void IsContext(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WeakCallback(const v8::WeakCallbackInfo<ContextifyContext>& data);
    static void PropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void PropertySetterCallback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void PropertyDescriptorCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void PropertyDefinerCallback(v8::Local<v8::Name> property, const v8::PropertyDescriptor& desc, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void PropertyDeleterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean>& args);
    static void PropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& args);
    static void IndexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void IndexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void IndexedPropertyDescriptorCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void IndexedPropertyDefinerCallback(uint32_t index, const v8::PropertyDescriptor& desc, const v8::PropertyCallbackInfo<v8::Value>& args);
    static void IndexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& args);
    V8VirtualMation* m_vm;
    Persistent<v8::Context> m_context;
};

class ContextifyScript : public BaseObject
{
public:
    ContextifyScript(V8VirtualMation* vm, v8::Local<v8::Object> object);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static bool InstanceOf(V8VirtualMation* vm, const v8::Local<v8::Value>& value);
    static void CreateCachedData(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RunInThisContext(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RunInContext(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void DecorateErrorStack(V8VirtualMation* vm, const v8::TryCatch& try_catch);
    static bool EvalMachine(V8VirtualMation* vm, const int64_t timeout, const bool display_errors, const bool break_on_sigint, const v8::FunctionCallbackInfo<v8::Value>& args);

    static void InitializeContextifyModule(V8VirtualMation* vm, v8::Local<v8::Object> target);
private:
    Persistent<v8::UnboundScript> m_script;
};
