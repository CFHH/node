#pragma once
#include "v8.h"
#include "vm_internal.h"

enum
{
    VM_F_BUILTIN = 1 << 0,
    VM_F_LINKED = 1 << 1,
    VM_F_INTERNAL = 1 << 2,
};

typedef void(*addon_register_func)(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, void* priv);
typedef void(*addon_context_register_func)(v8::Local<v8::Object> exports, v8::Local<v8::Value> module, v8::Local<v8::Context> context, void* priv);

struct vm_module
{
    int vm_version;
    unsigned int vm_flags;
    const char* vm_filename;
    addon_register_func vm_register_func;
    addon_context_register_func vm_context_register_func;
    const char* vm_modname;
    void* vm_priv;
    struct vm_module* vm_link;
};


#define NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, priv, flags)          \
  static vm_module _module = {                                                \
    VM_MODULE_VERSION,                                                        \
    flags,                                                                    \
    __FILE__,                                                                 \
    nullptr,                                                                  \
    (addon_context_register_func) (regfunc),                                  \
    #modname,                                                                 \
    priv,                                                                     \
    nullptr                                                                   \
  };                                                                          \
  void _register_ ## modname() {                                              \
    vm_module_register(&_module);                                             \
  }

#define NODE_BUILTIN_MODULE_CONTEXT_AWARE(modname, regfunc)                   \
  NODE_MODULE_CONTEXT_AWARE_CPP(modname, regfunc, nullptr, VM_F_BUILTIN)

#define NODE_BUILTIN_MODULES(V)     \
    V(testmod)

void vm_module_register(void* mod);

void RegisterBuiltinModules();
