#include "module.h"

static bool vm_is_initialized = false;
static vm_module* modpending = NULL;
static vm_module* modlist_builtin = NULL;
static vm_module* modlist_internal = NULL;
static vm_module* modlist_linked = NULL;
static vm_module* modlist_addon = NULL;

void vm_module_register(void* m)
{
    struct vm_module* mp = reinterpret_cast<struct vm_module*>(m);

    if (mp->vm_flags & VM_F_BUILTIN)
    {
        mp->vm_link = modlist_builtin;
        modlist_builtin = mp;
    }
    else if (mp->vm_flags & VM_F_INTERNAL)
    {
        mp->vm_link = modlist_internal;
        modlist_internal = mp;
    }
    else if (!vm_is_initialized)
    {
        mp->vm_flags = VM_F_LINKED;
        mp->vm_link = modlist_linked;
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
    vm_is_initialized = true;
}
