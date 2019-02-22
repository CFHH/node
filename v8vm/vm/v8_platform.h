#pragma once
#include "v8.h"
#include "v8-platform.h"

class V8Platform : public v8::Platform
{
public:
    virtual ~V8Platform() {}

    //以下都是扩展的函数，非v8::Platform
    //virtual void RegisterIsolate(IsolateData* isolate_data, struct uv_loop_s* loop) = 0;
    //virtual void UnregisterIsolate(IsolateData* isolate_data) = 0;
};
