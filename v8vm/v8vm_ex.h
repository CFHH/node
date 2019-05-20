#pragma once
#include "v8.h"
#include "platform.h"

void BalanceTransfer_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);
void UpdateDB_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);
void QueryDB_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);
void RequireAuth_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);

void LoadScript_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);

void GetSourcePath_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);

void IsFileExists_JS2C(const v8::FunctionCallbackInfo<v8::Value>& args);


