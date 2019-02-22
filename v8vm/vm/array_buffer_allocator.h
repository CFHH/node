#pragma once
#include "platform.h"
#include "v8.h"

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator
{
public:
    inline Uint32* zero_fill_field() { return &m_zero_fill_field; }

    virtual void* Allocate(size_t size);
    virtual void* AllocateUninitialized(size_t size);
    virtual void Free(void* data, size_t);

private:
    Uint32 m_zero_fill_field = 1;
};
