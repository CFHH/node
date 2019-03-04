#pragma once
#include "v8.h"

class V8VirtualMation;

void DefineConstants(V8VirtualMation* vm, v8::Local<v8::Object> exports);
