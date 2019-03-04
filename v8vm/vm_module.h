#pragma once
#include "v8.h"
#include "vm_util.h"

#define VM_MODULE_VERSION 64

enum
{
    VM_F_BUILTIN = 1 << 0,
    VM_F_INTERNAL = 1 << 1,
    VM_F_LINKED = 1 << 2,
};

typedef void(*addon_register_func)(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, void* private_data);
typedef void(*addon_context_register_func)(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* private_data);

struct VMModule
{
    int m_vm_version;
    unsigned int m_vm_flags;
    const char* m_vm_modname;
    const char* m_vm_filename;
    addon_register_func m_vm_register_func;  //NODE_MODULE_X
    addon_context_register_func m_vm_context_register_func;
    void* m_vm_private_data;
    VMModule* m_vm_link;
};

void RegisterVMModule(VMModule* module);

void RegisterBuiltinModules();

void GetBinding(const v8::FunctionCallbackInfo<v8::Value>& args);
void GetLinkedBinding(const v8::FunctionCallbackInfo<v8::Value>& args);
void GetInternalBinding(const v8::FunctionCallbackInfo<v8::Value>& args);



#define NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, private_data, flags)    \
    static VMModule _module = {                                                 \
        VM_MODULE_VERSION,                                                      \
        flags,                                                                  \
        STRINGIFY(modname),                                                     \
        __FILE__,                                                               \
        nullptr,                                                                \
        (addon_context_register_func) (regfunc),                                \
        private_data,                                                           \
        nullptr                                                                 \
    };                                                                          \
    void _register_ ## modname() {                                              \
        RegisterVMModule(&_module);                                             \
    }

#define NODE_BUILTIN_MODULE_CONTEXT_AWARE(modname, regfunc)                     \
    NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, nullptr, VM_F_BUILTIN)

#define NODE_MODULE_CONTEXT_AWARE_INTERNAL(modname, regfunc)                    \
    NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, nullptr, VM_F_INTERNAL)

#define FOREACH_VM_BUILTIN_MODULES(V)   \
    V(testmod)                          \
    /*builtin*/                         \
    V(util)                             \
    /*internals*/                       \
    V(symbols)
