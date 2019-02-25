#include "array_buffer_allocator.h"
#include "vm_util.h"

void* ArrayBufferAllocator::Allocate(size_t size)
{
    if (m_zero_fill_field)
        return UncheckedCalloc(size);
    else
        return UncheckedMalloc(size);
}

void* ArrayBufferAllocator::AllocateUninitialized(size_t size)
{
    return UncheckedMalloc(size);
}

void ArrayBufferAllocator::Free(void* data, size_t)
{
    free(data);
}
