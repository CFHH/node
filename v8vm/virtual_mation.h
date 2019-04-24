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

#define PER_ISOLATE_PRIVATE_SYMBOL_PROPERTIES(V)                    \
    V(arrow_message_private_symbol, "v8vm:arrowMessage")            \
    V(contextify_context_private_symbol, "v8vm:contextify:context") \
    V(contextify_global_private_symbol, "v8vm:contextify:global")   \
    V(decorated_private_symbol, "v8vm:decorated")

#define PER_ISOLATE_SYMBOL_PROPERTIES(V)                            \
    V(handle_onclose_symbol, "handle_onclose")

#define PER_ISOLATE_STRING_PROPERTIES(V)                            \
    V(cached_data_produced_string, "cachedDataProduced")            \
    V(cached_data_rejected_string, "cachedDataRejected")            \
    V(cached_data_string, "cachedData")                             \
    V(fatal_exception_string, "_fatalException")                    \
    V(message_string, "message")                                    \
    V(stack_string, "stack")

#define ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)                 \
    V(as_external, v8::External)                                    \
    V(buffer_prototype_object, v8::Object)                          \
    V(context, v8::Context)                                         \
    V(process_object, v8::Object)                                   \
    V(script_context_constructor_template, v8::FunctionTemplate)    \
    V(script_data_constructor_function, v8::Function)               \
    V(vm_parsing_context_symbol, v8::Symbol)

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

    void AssignToContext(v8::Local<v8::Context> context);
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

    void ReportException(v8::Local<v8::Value> er, v8::Local<v8::Message> message);
    void ReportException(const v8::TryCatch& try_catch);

    bool IsExceptionDecorated(v8::Local<v8::Value> er);
    void AppendExceptionLine(v8::Local<v8::Value> er, v8::Local<v8::Message> message, enum ErrorHandlingMode mode);
    bool CanCallIntoJS() { return m_can_call_into_js; }
    bool SetCanCallIntoJS(bool value) { m_can_call_into_js = value; }


private:
    void SetupProcessObject();
    void SetupBootstrapObject(v8::Local<v8::Object> bootstrapper);
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
    bool m_can_call_into_js = true;

public:
    void AddCleanupHook(void(*fn)(void*), void* arg);
    void RemoveCleanupHook(void(*fn)(void*), void* arg);
    void RunCleanup();
private:
    struct CleanupHookCallback
    {
        void(*m_fn)(void*);
        void* m_arg;
        // We keep track of the insertion order for these objects, so that we can call the callbacks in reverse order when we are cleaning up.
        uint64_t m_insertion_order_counter;
        struct Hash
        {
            size_t operator()(const CleanupHookCallback& cb) const
            {
                return std::hash<void*>()(cb.m_arg);
            }
        };
        struct Equal
        {
            bool operator()(const CleanupHookCallback& a, const CleanupHookCallback& b) const
            {
                return a.m_fn == b.m_fn && a.m_arg == b.m_arg;
            }
        };
    };
    std::unordered_set<CleanupHookCallback, CleanupHookCallback::Hash, CleanupHookCallback::Equal> m_cleanup_hooks;
    uint64_t m_cleanup_hook_counter = 0;

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

public:
    class ShouldNotAbortOnUncaughtScope
    {
    public:
        explicit ShouldNotAbortOnUncaughtScope(V8VirtualMation* vm) : m_vm(vm)
        {
            m_vm->m_should_not_abort_scope_counter++;
        }
        ~ShouldNotAbortOnUncaughtScope()
        {
            Close();
        }
        void Close()
        {
            if (m_vm != nullptr)
            {
                m_vm->m_should_not_abort_scope_counter--;
                m_vm = nullptr;
            }
        }
    private:
        V8VirtualMation * m_vm;
    };
    bool inside_should_not_abort_on_uncaught_scope() const { return m_should_not_abort_scope_counter > 0; }
private:
    int m_should_not_abort_scope_counter = 0;
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
