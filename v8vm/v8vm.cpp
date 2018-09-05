#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libplatform/libplatform.h"
#include "v8.h"
#include "v8vm.h"

bool gVMInitialized = false;
std::unique_ptr<v8::Platform> gPlatform;
v8::Isolate::CreateParams gIsolateCreateParams;
v8::Isolate* gIsolate = NULL;
txansaction_callback_fcn TransactionCallback = NULL;
txansaction_result TransactionResult = NULL;
txansaction_result_void TransactionResultVoid = NULL;

void TransactionCallbackCStub(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (args.Length() < 2)
		return;
	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);
	int value1 = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	v8::String::Utf8Value value2(args.GetIsolate(), args[1]);
	int result = TransactionCallback(value1, *value2);
	args.GetReturnValue().Set(result);
}

const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
	//bool first = true;
	//for (int i = 0; i < args.Length(); i++) {
	//	v8::HandleScope handle_scope(args.GetIsolate());
	//	if (first) {
	//		first = false;
	//	}
	//	else {
	//		printf(" ");
	//	}
	//	v8::String::Utf8Value str(args.GetIsolate(), args[i]);
	//	const char* cstr = ToCString(str);
	//	printf("%s", cstr);
	//}
	//printf("\n");
	//fflush(stdout);
}

V8VM_EXPORT void __stdcall InitializeVM()
{
	if (gVMInitialized)
		return;
	gVMInitialized = true;
	v8::V8::InitializeICUDefaultLocation(nullptr);
	v8::V8::InitializeExternalStartupData(nullptr);
	gPlatform = v8::platform::NewDefaultPlatform();
	v8::V8::InitializePlatform(gPlatform.get());
	v8::V8::Initialize();
	gIsolateCreateParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	gIsolate = v8::Isolate::New(gIsolateCreateParams);

	//v8::Persistent<v8::ObjectTemplate> global = v8::ObjectTemplate::New(gIsolate);
	//v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(gIsolate);
	//v8::Persistent<v8::ObjectTemplate> persistent_global(gIsolate, global);
	//persistent_global->Set(v8::String::NewFromUtf8(gIsolate, "TransactionCallback", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(gIsolate, TransactionCallbackCStub));
}

V8VM_EXPORT void __stdcall DisposeVM()
{
	gIsolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete gIsolateCreateParams.array_buffer_allocator;
}

V8VM_EXPORT int __stdcall ExecuteTransaction(const char *jssrc)
{
	//首先要找到一个合适的虚拟机Isolate，需要传递参数
	v8::Isolate::Scope isolate_scope(gIsolate);

	//此函数中，接下来的handle都会被HandleScope管理，析构时删除Handle
	v8::HandleScope handle_scope(gIsolate);

	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(gIsolate);
	global->Set(v8::String::NewFromUtf8(gIsolate, "TransactionCallback", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(gIsolate, TransactionCallbackCStub));
	global->Set(
		v8::String::NewFromUtf8(gIsolate, "print1", v8::NewStringType::kNormal)
		.ToLocalChecked(),
		v8::FunctionTemplate::New(gIsolate, Print));

	//接下来找到合适的智能合约Context
	//为每个虚拟机里的每个智能合约脚本建立一个v8::Persistent<v8::Context> persistent_context = v8::Context::New(gIsolate);
	//不再需要这个智能合约而需要回收时，使用PersistentBase::SetWeak。Persistent::New, Persistent::Dispose？
	v8::Local<v8::Context> context = v8::Context::New(gIsolate, NULL, global);

	//Context::Scope用来管理Context对象，在构造中调用context->Enter()，而在析构函数中调用context->Leave()
	v8::Context::Scope context_scope(context);

	// Create a string containing the JavaScript source code.
	v8::Local<v8::String> source = v8::String::NewFromUtf8(gIsolate, jssrc, v8::NewStringType::kNormal).ToLocalChecked();

	// Compile the source code.
	v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();

	// Run the script to get the result.
	v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

	// Convert the result to an UTF8 string and print it.
	v8::String::Utf8Value utf8(gIsolate, result);

	// Callback
	int result2 = 0;
	if (TransactionResultVoid != NULL)
		TransactionResultVoid(0, *utf8);
	if (TransactionResult != NULL)
		result2 = TransactionResult(0, *utf8);

	// Return.
	return result2;
}

V8VM_EXPORT int __stdcall AddFunc(int a, int b)
{
	return a + b;
}

V8VM_EXPORT void __stdcall SetTransactionCallback(txansaction_callback_fcn callback)
{
	TransactionCallback = callback;
}

V8VM_EXPORT void __stdcall SetTransactionResult(txansaction_result callback)
{
	TransactionResult = callback;
}

V8VM_EXPORT void __stdcall SetTransactionResultVoid(txansaction_result_void callback)
{
	TransactionResultVoid = callback;
}
