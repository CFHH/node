#include "vm_module.h"

static bool modules_are_initialized = false;
static VMModule* modpending = NULL;
static VMModule* modlist_builtin = NULL;
static VMModule* modlist_internal = NULL;
static VMModule* modlist_linked = NULL;
static VMModule* modlist_addon = NULL;

void VMModuleRegister(void* m)
{
    VMModule* mp = reinterpret_cast<VMModule*>(m);

    if (mp->m_vm_flags & VM_F_BUILTIN)
    {
        mp->m_vm_link = modlist_builtin;
        modlist_builtin = mp;
    }
    else if (mp->m_vm_flags & VM_F_INTERNAL)
    {
        mp->m_vm_link = modlist_internal;
        modlist_internal = mp;
    }
    else if (!modules_are_initialized)
    {
        mp->m_vm_flags = VM_F_LINKED;
        mp->m_vm_link = modlist_linked;
        modlist_linked = mp;
    }
    else
    {
        modpending = mp;
    }
}

#define V(modname) void _register_##modname();
    NODE_BUILTIN_MODULES(V)
#undef V

void RegisterBuiltinModules()
{
#define V(modname) _register_##modname();
    NODE_BUILTIN_MODULES(V)
#undef V
    modules_are_initialized = true;
}
