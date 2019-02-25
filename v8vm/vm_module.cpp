#include "vm_module.h"

static bool modules_are_initialized = false;
static VMModule* modlist_builtin = NULL;
static VMModule* modlist_internal = NULL;
static VMModule* modlist_linked = NULL;
static VMModule* modlist_addon = NULL;
static VMModule* modpending = NULL;

void RegisterVMModule(VMModule* module)
{
    if (module->m_vm_flags & VM_F_BUILTIN)
    {
        module->m_vm_link = modlist_builtin;
        modlist_builtin = module;
    }
    else if (module->m_vm_flags & VM_F_INTERNAL)
    {
        module->m_vm_link = modlist_internal;
        modlist_internal = module;
    }
    else if (!modules_are_initialized)
    {
        module->m_vm_flags = VM_F_LINKED;
        module->m_vm_link = modlist_linked;
        modlist_linked = module;
    }
    else
    {
        modpending = module;
    }
}

#define V(modname) void _register_##modname();
    FOREACH_VM_BUILTIN_MODULES(V)
#undef V

void RegisterBuiltinModules()
{
#define V(modname) _register_##modname();
    FOREACH_VM_BUILTIN_MODULES(V)
#undef V
    modules_are_initialized = true;
}
