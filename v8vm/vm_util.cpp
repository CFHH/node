#include "v8.h"
#include "vm_util.h"

NO_RETURN void Abort()
{
    ABORT_NO_BACKTRACE();
}

NO_RETURN void Assert(const char* const (*args)[4])
{
    const char* filename = (*args)[0];
    const char* linenum = (*args)[1];
    const char* message = (*args)[2];
    const char* function = (*args)[3];
    //ZZWTODO 日志
    Abort();
}

void LowMemoryNotification()
{
    if (v8_initialized)
    {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        if (isolate != nullptr)
            isolate->LowMemoryNotification();
    }
}
