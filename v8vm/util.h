#pragma once

#include <map>
#include <string>
#include "libplatform/libplatform.h"
#include "v8.h"

struct InvokeParam;

char* V8ObjectToCString(v8::Isolate* isolate, v8::Local<v8::Value> value);
std::string V8ObjectToString(v8::Isolate* isolate, v8::Local<v8::Value> value);
std::map<std::string, std::string>* V8Object2Map(v8::Local<v8::Object> obj);
InvokeParam* V8Object2InvokeParam(v8::Local<v8::Object> obj);

void MapGet_JS2C(v8::Local<v8::Name> key_obj, const v8::PropertyCallbackInfo<v8::Value>& info);
void MapSet_JS2C(v8::Local<v8::Name> key_obj, v8::Local<v8::Value> value_obj, const v8::PropertyCallbackInfo<v8::Value>& info);
void GetInvokeParam1_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info);
void GetInvokeParam2_JS2C(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info);

void ReportV8Exception(v8::Isolate* isolate, v8::TryCatch* try_catch);


void Log_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);

void Log(const char *format, ...);
bool ReadScriptFile(const char* filename, char*& buf);
