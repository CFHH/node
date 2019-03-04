#include "vm_module.h"
#include "virtual_mation.h"
#include "module/v8vm_constants.h"
#include "module/v8vm_javascript.h"
#include "module/v8vm_code_cache.h"

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

struct VMModule* FindModule(VMModule* list, const char* name, int flag)
{
    VMModule* module;
    for (module = list; module != nullptr; module = module->m_vm_link)
    {
        if (strcmp(module->m_vm_modname, name) == 0)
            break;
    }
    CHECK(module == nullptr || (module->m_vm_flags & flag) != 0);
    return module;
}

VMModule* get_builtin_module(const char* name)
{
    return FindModule(modlist_builtin, name, VM_F_BUILTIN);
}
VMModule* get_internal_module(const char* name)
{
    return FindModule(modlist_internal, name, VM_F_INTERNAL);
}
VMModule* get_linked_module(const char* name)
{
    return FindModule(modlist_linked, name, VM_F_LINKED);
}

v8::Local<v8::Object> InitModule(v8::Isolate* isolate, v8::Local<v8::Context> context, VMModule* module)
{
    v8::Local<v8::Object> exports = v8::Object::New(isolate);
    CHECK_NULL(module->m_vm_register_func);
    CHECK_NOT_NULL(module->m_vm_context_register_func);
    v8::Local<v8::Value> unused = v8::Undefined(isolate);
    module->m_vm_context_register_func(exports, unused, context, module->m_vm_private_data);
    return exports;
}

void ThrowIfNoSuchModule(V8VirtualMation* vm, const char* module_name)
{
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg), "No such module: %s", module_name);
    vm->ThrowError(errmsg);
}

void GetBinding(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK(args[0]->IsString());
    v8::Local<v8::String> name = args[0].As<v8::String>();
    v8::String::Utf8Value utf8_value(isolate, name);
    char* module_name = *utf8_value;

    VMModule* module = get_builtin_module(module_name);
    v8::Local<v8::Object> exports;
    if (module != nullptr)
    {
        exports = InitModule(isolate, context, module);
    }
    else if (!strcmp(module_name, "constants"))
    {
        exports = v8::Object::New(isolate);
        CHECK(exports->SetPrototype(context, v8::Null(isolate)).FromJust());
        DefineConstants(vm, exports);
    }
    else if (!strcmp(module_name, "natives"))
    {
        exports = v8::Object::New(isolate);
        DefineJavaScript(vm, exports);
    }
    else
    {
        ThrowIfNoSuchModule(vm, module_name);
        return;
    }

    args.GetReturnValue().Set(exports);
}

void GetLinkedBinding(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK(args[0]->IsString());
    v8::Local<v8::String> name = args[0].As<v8::String>();
    v8::String::Utf8Value utf8_value(isolate, name);
    char* module_name = *utf8_value;

    VMModule* module = get_linked_module(module_name);
    if (module == nullptr)
    {
        char errmsg[1024];
        snprintf(errmsg, sizeof(errmsg), "No such module was linked: %s", module_name);
        vm->ThrowError(errmsg);
        return;
    }

    v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
    v8::Local<v8::Object> exports = v8::Object::New(isolate);
    v8::Local<v8::String> exports_prop = v8::String::NewFromUtf8(isolate, "exports");
    module_obj->Set(exports_prop, exports);

    if (module->m_vm_context_register_func != nullptr)
    {
        module->m_vm_context_register_func(exports, module_obj, context, module->m_vm_private_data);
    }
    else if (module->m_vm_register_func != nullptr)
    {
        module->m_vm_register_func(exports, module_obj, module->m_vm_private_data);
    }
    else
    {
        return vm->ThrowError("Linked module has no declared entry point.");
    }

    auto effective_exports = module_obj->Get(exports_prop);

    args.GetReturnValue().Set(effective_exports);
}

void GetInternalBinding(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    V8VirtualMation* vm = V8VirtualMation::GetCurrent(args);
    v8::Isolate* isolate = vm->GetIsolate();
    v8::Local<v8::Context> context = vm->context();

    CHECK(args[0]->IsString());
    v8::Local<v8::String> name = args[0].As<v8::String>();
    v8::String::Utf8Value utf8_value(isolate, name);
    char* module_name = *utf8_value;

    VMModule* module = get_internal_module(module_name);
    v8::Local<v8::Object> exports;
    if (module != nullptr)
    {
        exports = InitModule(isolate, context, module);
    }
    else if (!strcmp(module_name, "code_cache"))
    {
        exports = v8::Object::New(isolate);
        DefineCodeCache(vm, exports);
    }
    else
    {
        ThrowIfNoSuchModule(vm, module_name);
        return;
    }

    args.GetReturnValue().Set(exports);
}
