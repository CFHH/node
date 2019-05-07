#include "v8vm_util.h"
#include <stdarg.h>
#include "smart_contract.h"
#include <string.h>

#define LOG_BUF_SIZE 1024

char* V8ObjectToCString(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
    v8::String::Utf8Value utf8_value(isolate, value);
    return *utf8_value;
}

std::string V8ObjectToString(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
    v8::String::Utf8Value utf8_value(isolate, value);
    return std::string(*utf8_value);
}

std::map<std::string, std::string>* V8Object2Map(v8::Local<v8::Object> obj)
{
    v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<std::map<std::string, std::string>*>(ptr);
}

InvokeParam* V8Object2InvokeParam(v8::Local<v8::Object> obj)
{
    v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<InvokeParam*>(ptr);
}

void MapGet_JS2C(v8::Local<v8::Name> key_obj, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (key_obj->IsSymbol())
        return;
    std::map<std::string, std::string>* obj = V8Object2Map(info.Holder());
    if (obj == NULL)
        return;
    v8::Isolate* isolate = info.GetIsolate();
    std::string key = V8ObjectToString(isolate, v8::Local<v8::String>::Cast(key_obj));
    std::map<std::string, std::string>::iterator iter = obj->find(key);
    if (iter == obj->end())
        return;
    const std::string& value = (*iter).second;
    info.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value.c_str(), v8::NewStringType::kNormal, static_cast<int>(value.length())).ToLocalChecked());
}

void MapSet_JS2C(v8::Local<v8::Name> key_obj, v8::Local<v8::Value> value_obj, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (key_obj->IsSymbol())
        return;
    std::map<std::string, std::string>* obj = V8Object2Map(info.Holder());
    if (obj == NULL)
        return;
    v8::Isolate* isolate = info.GetIsolate();
    std::string key = V8ObjectToString(isolate, v8::Local<v8::String>::Cast(key_obj));
    std::string value = V8ObjectToString(isolate, value_obj);
    (*obj)[key] = value;
    info.GetReturnValue().Set(value_obj);
}

void GetInvokeParam_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    InvokeParam* param = V8Object2InvokeParam(info.Holder());
    if (param == NULL)
        return;
    v8::String::Utf8Value utf8_value(info.GetIsolate(), v8::Local<v8::String>::Cast(name));
    char* field = *utf8_value;
    if (strcmp(field, INVOKE_PARAM_PARAM0) == 0)
        info.GetReturnValue().Set(param->param0);
    else if (strcmp(field, INVOKE_PARAM_PARAM1) == 0)
        info.GetReturnValue().Set(param->param1);
    else if (strcmp(field, INVOKE_PARAM_PARAM2) == 0)
        info.GetReturnValue().Set(v8::String::NewFromUtf8(info.GetIsolate(), param->param2.c_str(), v8::NewStringType::kNormal, static_cast<int>(param->param2.length())).ToLocalChecked());
}

void GetInvokeParam0_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    InvokeParam* param = V8Object2InvokeParam(info.Holder());
    if (param == NULL)
        return;
    info.GetReturnValue().Set(param->param0);
}

void GetInvokeParam1_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    InvokeParam* param = V8Object2InvokeParam(info.Holder());
    if (param == NULL)
        return;
    info.GetReturnValue().Set(param->param1);
}

void GetInvokeParam2_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    InvokeParam* param = V8Object2InvokeParam(info.Holder());
    if (param == NULL)
        return;
    info.GetReturnValue().Set(v8::String::NewFromUtf8(info.GetIsolate(), param->param2.c_str(), v8::NewStringType::kNormal, static_cast<int>(param->param2.length())).ToLocalChecked());
}

std::string ExceptionString(v8::Isolate* isolate, v8::TryCatch* try_catch)
{
    //ZZWTODO vm->stack_string() vm->arrow_message_private_symbol()
    std::string out = "V8VM_EXCEPTION:\r\n";
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context(isolate->GetCurrentContext());

    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    const char* exception_string = *exception;
    if (exception_string == NULL)
        exception_string = "<unknown exception>";
    v8::Local<v8::Message> message = try_catch->Message();
    if (message.IsEmpty())
    {
        out.append(exception_string);
        return out;
    }

    const size_t tempbuf_size = 20;
    char tempbuf[tempbuf_size];

    //Print (filename):(line number): (exception)
    v8::String::Utf8Value filename(isolate, message->GetScriptOrigin().ResourceName());
    const char* filename_string = *filename;
    if (filename_string == NULL)
        filename_string = "<unknown script file name>";
    int linenum = message->GetLineNumber(context).FromJust();
    snprintf(tempbuf, tempbuf_size, "%i", linenum);
    out.append(filename_string);
    out.append(" : ");
    out.append(tempbuf);
    out.append(".\r\n");

    //Print line of source code
    v8::String::Utf8Value sourceline(isolate, message->GetSourceLine(context).ToLocalChecked());
    const char* sourceline_string = *sourceline;
    if (sourceline_string == NULL)
        sourceline_string = "<unknown source line>";
    out.append(sourceline_string);
    out.append("\r\n");

    //Print wavy underline
    int start = message->GetStartColumn(context).FromJust();
    int end = message->GetEndColumn(context).FromJust();
    char* wave = new char[end + 1];
    for (int i = 0; i < start; i++)
        wave[i] = ' ';
    for (int i = start; i < end; i++)
        wave[i] = '^';
    wave[end] = 0;
    out.append(wave);
    out.append("\r\n");
    delete wave;

    //Print stack trace
    v8::Local<v8::Value> stack_trace_string;
    if (try_catch->StackTrace(context).ToLocal(&stack_trace_string)
        && stack_trace_string->IsString()
        && v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0
        )
    {
        v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
        const char* stack_trace_string = *stack_trace;
        if (stack_trace_string == NULL)
            stack_trace_string = "<unknown stack trace>";
        out.append(stack_trace_string);
        out.append("\r\n");
    }
    return out;
}

void ReportV8Exception(v8::Isolate* isolate, v8::TryCatch* try_catch)
{
    std::string exception_string = ExceptionString(isolate, try_catch);
    Log(exception_string.c_str());
}

void Log_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1)
        return;
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Value> arg = args[0];
    v8::String::Utf8Value value(isolate, arg);
    Log(*value);
}

void Log(const char *format, ...)
{
    char buf[LOG_BUF_SIZE] = { 0 };
    va_list argptr;
    va_start(argptr, format);
#ifdef _MSC_VER
    vsprintf_s(buf, LOG_BUF_SIZE - 1, format, argptr);
#else
    vsprintf(buf, format, argptr);
#endif
    va_end(argptr);
    printf("%s\n", buf);
}

bool ReadScriptFile(const char* filename, std::string& out)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
        return false;
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    out.resize(size);
    for (size_t i = 0; i < size;)
    {
        i += fread(&out[i], 1, size - i, file);
        if (ferror(file))
        {
            fclose(file);
            return false;
        }
    }
    fclose(file);
    return true;
}
