#pragma once

#include <map>
#include "libplatform/libplatform.h"
#include "v8.h"
#include "platform.h"
#include "v8vm_internal.h"
#include "vm_util.h"

class V8Environment;
class SmartContract;
struct InvokeParam;

#define PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(V)        \
    V(alpn_buffer_private_symbol, "alpnBuffer")

#define PER_ISOLATE_SYMBOL_PROPERTIES(V)                \
    V(handle_onclose_symbol, "handle_onclose")

#define PER_ISOLATE_STRING_PROPERTIES(V)                \
    V(address_string, "address")

#define ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)     \
    V(as_external, v8::External)                        \
    V(context, v8::Context)                             \
    V(process_object, v8::Object)

class V8VirtualMation
{
    DISALLOW_COPY_AND_ASSIGN(V8VirtualMation);
public:
    explicit V8VirtualMation(V8Environment* environment, Int64 vmid);
    ~V8VirtualMation();
    v8::Isolate* GetIsolate() { return m_isolate; }
    Int64 VMID() { return m_vmid; }
    bool IsInUse();
    void PumpMessage();

    SmartContract* CreateSmartContract(const char* contract_name, const char* sourcecode);
    void DestroySmartContract(const char* contract_name);
    SmartContract* GetSmartContract(const char* contract_name);

    void InstallMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* stdmap, const char* map_name);

    v8::Local<v8::Object> WrapInvokeParam(v8::Local<v8::Context> context, InvokeParam* param);

    static V8VirtualMation* GetCurrent(v8::Isolate* isolate);
    static V8VirtualMation* GetCurrent(v8::Local<v8::Context> context);
    static V8VirtualMation* GetCurrent(const v8::FunctionCallbackInfo<v8::Value>& info);
    template <typename T>
    static V8VirtualMation* GetCurrent(const v8::PropertyCallbackInfo<T>& info);

    void SetReadOnlyProperty(v8::Local<v8::Object> obj, const char* key, const char* val);

    v8::Local<v8::FunctionTemplate> NewFunctionTemplate(v8::FunctionCallback callback,
        v8::Local<v8::Signature> signature = v8::Local<v8::Signature>(),
        v8::ConstructorBehavior behavior = v8::ConstructorBehavior::kAllow,
        v8::SideEffectType side_effect = v8::SideEffectType::kHasSideEffect);
    void SetMethod(v8::Local<v8::Object> that, const char* name, v8::FunctionCallback callback);
    void SetProtoMethod(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback);
    void SetTemplateMethod(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback);
    void SetMethodNoSideEffect(v8::Local<v8::Object> that, const char* name, v8::FunctionCallback callback);
    void SetProtoMethodNoSideEffect(v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback);
    void SetTemplateMethodNoSideEffect( v8::Local<v8::FunctionTemplate> that, const char* name, v8::FunctionCallback callback);

    void ThrowError(const char* errmsg);
    void ThrowTypeError(const char* errmsg);
    void ThrowRangeError(const char* errmsg);
    void ThrowError(v8::Local<v8::Value>(*fun)(v8::Local<v8::String>), const char* errmsg);

private:
    void SetupProcessObject();
    void LoadEnvironment();
    v8::Local<v8::Object> WrapMap(v8::Local<v8::Context> context, std::map<std::string, std::string>* obj);
    v8::Local<v8::ObjectTemplate> MakeMapTemplate();
    v8::Local<v8::ObjectTemplate> MakeInvokeParamTemplate();

private:
    V8Environment* m_environment;
    Int64 m_vmid;
    v8::Isolate::CreateParams m_create_params;
    v8::Isolate* m_isolate;
    std::map<std::string, std::string> m_output;
    v8::Global<v8::ObjectTemplate> m_map_template;
    v8::Global<v8::ObjectTemplate> m_invoke_param_template;
    std::map<std::string, SmartContract*> m_contracts;

public:
#define VP(PropertyName, StringValue) V(v8::Private, PropertyName)
#define VY(PropertyName, StringValue) V(v8::Symbol, PropertyName)
#define VS(PropertyName, StringValue) V(v8::String, PropertyName)
#define V(TypeName, PropertyName)   \
    inline v8::Local<TypeName> PropertyName() const;
PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(VP)
PER_ISOLATE_SYMBOL_PROPERTIES(VY)
PER_ISOLATE_STRING_PROPERTIES(VS)
#undef V
#undef VS
#undef VY
#undef VP

private:
#define VP(PropertyName, StringValue) V(v8::Private, PropertyName)
#define VY(PropertyName, StringValue) V(v8::Symbol, PropertyName)
#define VS(PropertyName, StringValue) V(v8::String, PropertyName)
#define V(TypeName, PropertyName)   \
    v8::Eternal<TypeName> m_ ## PropertyName;
    PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(VP)
    PER_ISOLATE_SYMBOL_PROPERTIES(VY)
    PER_ISOLATE_STRING_PROPERTIES(VS)
#undef V
#undef VS
#undef VY
#undef VP

public:
#define V(PropertyName, TypeName)                       \
    inline v8::Local<TypeName> PropertyName() const;    \
    inline void set_ ## PropertyName(v8::Local<TypeName> value);
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V

private:
#define V(PropertyName, TypeName) Persistent<TypeName> m_ ## PropertyName;
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V
};


#define VP(PropertyName, StringValue) V(v8::Private, PropertyName)
#define VY(PropertyName, StringValue) V(v8::Symbol, PropertyName)
#define VS(PropertyName, StringValue) V(v8::String, PropertyName)
#define V(TypeName, PropertyName)                                       \
    inline v8::Local<TypeName> V8VirtualMation::PropertyName() const    \
    {                                                                   \
        return m_ ## PropertyName.Get(m_isolate);                       \
    }
PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(VP)
PER_ISOLATE_SYMBOL_PROPERTIES(VY)
PER_ISOLATE_STRING_PROPERTIES(VS)
#undef V
#undef VS
#undef VY
#undef VP


#define V(PropertyName, TypeName)                                                   \
    inline v8::Local<TypeName> V8VirtualMation::PropertyName() const                \
    {                                                                               \
        return StrongPersistentToLocal(m_ ## PropertyName);                         \
    }                                                                               \
    inline void V8VirtualMation::set_ ## PropertyName(v8::Local<TypeName> value)    \
    {                                                                               \
        m_ ## PropertyName.Reset(m_isolate, value);                                 \
    }
ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V
