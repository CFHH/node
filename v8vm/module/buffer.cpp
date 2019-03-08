#include "buffer.h"
#include "virtual_mation.h"
#include "vm_util.h"

bool zero_fill_all_buffers = false;

void* BufferMalloc(size_t length)
{
    return zero_fill_all_buffers ? UncheckedCalloc(length) : UncheckedMalloc(length);
}

v8::MaybeLocal<v8::Uint8Array> Buffer::New(V8VirtualMation* vm, v8::Local<v8::ArrayBuffer> ab, size_t byte_offset, size_t length)
{
    v8::Local<v8::Uint8Array> ui = v8::Uint8Array::New(ab, byte_offset, length);
    v8::Maybe<bool> mb = ui->SetPrototype(vm->context(), vm->buffer_prototype_object());
    if (mb.IsNothing())
        return v8::MaybeLocal<v8::Uint8Array>();
    return ui;
}

v8::MaybeLocal<v8::Object> Buffer::New(V8VirtualMation* vm, size_t length)
{
    if (length > kMaxLength)
        return v8::Local<v8::Object>();
    v8::Isolate* isolate = vm->GetIsolate();
    v8::EscapableHandleScope scope(isolate);

    void* data;
    if (length > 0)
    {
        data = UncheckedMalloc(length);
        if (data == nullptr)
            return v8::Local<v8::Object>();
    }
    else
    {
        data = nullptr;
    }
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, data, length, v8::ArrayBufferCreationMode::kInternalized);
    v8::MaybeLocal<v8::Uint8Array> ui = Buffer::New(vm, ab, 0, length);
    if (ui.IsEmpty())
        free(data);

    return scope.Escape(ui.FromMaybe(v8::Local<v8::Uint8Array>()));
}

v8::MaybeLocal<v8::Object> Buffer::Copy(V8VirtualMation* vm, const char* data, size_t length)
{
    if (length > kMaxLength)
        return v8::Local<v8::Object>();
    v8::Isolate* isolate = vm->GetIsolate();
    v8::EscapableHandleScope scope(isolate);

    void* new_data;
    if (length > 0)
    {
        CHECK_NOT_NULL(data);
        new_data = UncheckedMalloc(length);
        if (new_data == nullptr)
            return v8::Local<v8::Object>();
        memcpy(new_data, data, length);
    }
    else
    {
        new_data = nullptr;
    }
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, new_data, length, v8::ArrayBufferCreationMode::kInternalized);
    v8::MaybeLocal<v8::Uint8Array> ui = Buffer::New(vm, ab, 0, length);
    if (ui.IsEmpty())
        free(new_data);

    return scope.Escape(ui.FromMaybe(v8::Local<v8::Uint8Array>()));
}
