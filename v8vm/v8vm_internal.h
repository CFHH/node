#pragma once
#include "v8.h"

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
