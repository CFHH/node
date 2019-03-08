#pragma once
#include "libplatform/libplatform.h"
#include "v8.h"
#include "platform.h"
#include <stdint.h>

class V8VirtualMation;
typedef void(*FreeCallback)(char* data, void* hint);

class Buffer
{
public:
    static const unsigned int kMaxLength = v8::TypedArray::kMaxLength;
    static v8::MaybeLocal<v8::Uint8Array> New(V8VirtualMation* vm, v8::Local<v8::ArrayBuffer> ab, size_t byte_offset, size_t length);
    static v8::MaybeLocal<v8::Object> New(V8VirtualMation* vm, size_t size);
    static v8::MaybeLocal<v8::Object> Copy(V8VirtualMation* vm, const char* data, size_t len);
};
