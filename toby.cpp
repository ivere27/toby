#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>

#include "node.h"

extern "C" void tobyOnLoad(void* isolate);
extern "C" char* tobyCall(const char* key, const char* value);

namespace {

using node::AtExit;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Number;
using v8::String;
using v8::HandleScope;
using v8::Local;
using v8::Value;
using v8::TryCatch;
using v8::Context;
using v8::Script;
using v8::Function;
using v8::NewStringType;

int add() {
  static int i = 0;
  return i++;
}

static Local<Value> GetValue(Isolate* isolate, Local<Context> context,
                             Local<Object> object, const char* property) {
  Local<String> v8_str =
      String::NewFromUtf8(isolate, property, NewStringType::kNormal)
          .ToLocalChecked();
  return object->Get(context, v8_str).ToLocalChecked();
}

static Local<Value> Stringify(Isolate* isolate, Local<Context> context,
                             Local<Value> object) {
  auto global = context->Global();

  Local<Value> result;
  auto JSON = GetValue(isolate, context, global, "JSON");
  auto stringify = GetValue(isolate, context, JSON.As<Object>(), "stringify");

  std::vector<Local<Value>> argv;
  argv.push_back(object);

  auto method = stringify.As<Function>();
  result = node::MakeCallback(isolate, global, method, argv.size(), argv.data());

  return result;
}

void AddMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  args.GetReturnValue().Set(add());
}

void HelloMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
}

void CallbackMethod(const FunctionCallbackInfo<Value>& args) {
  auto isolate = args.GetIsolate();
  Local<Value> result;

  if (args[0]->IsFunction()) {
    std::vector<Local<Value>> argv;
    Local<Value> argument = Number::New(isolate, add());
    argv.push_back(argument);

    auto method = args[0].As<Function>();
    result = node::MakeCallback(isolate,
      isolate->GetCurrentContext()->Global(), method,
      argv.size(), argv.data());
  }
  else {
    result = Undefined(isolate);
  }

  args.GetReturnValue().Set(result);
}


void CompileMethod(const FunctionCallbackInfo<Value>& args) {
  auto isolate = args.GetIsolate();
  Local<Value> result;

  const char* source = "function __c() {"
                       "  this.x = 42;"
                       "};"
                       "var __val = 43;";

  HandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);

  Local<Context> context(isolate->GetCurrentContext());

  Local<String> script = String::NewFromUtf8(isolate, source);
  Local<Script> compiled_script;
  if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
    String::Utf8Value error(try_catch.Exception());
    return;
  }

  if (!compiled_script->Run(context).ToLocal(&result)) {
    String::Utf8Value error(try_catch.Exception());
    return;
  }

  args.GetReturnValue().Set(result);
}

void GlobalGetMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);

  auto context = isolate->GetCurrentContext();
  Local<Object> global = context->Global();


  Local<Value> value = GetValue(isolate, context, global, "bar");
  if (value->IsFunction()) {
    printf("c++ : bar is function\n");
  }

  args.GetReturnValue().Set(value->IsFunction());
}

void CallMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Value> result;

  auto context = isolate->GetCurrentContext();
  auto global = context->Global();

  {
    result = Stringify(isolate, context, args[1]);

    v8::String::Utf8Value key(args[0]);
    const char* c_key = *key;

    v8::String::Utf8Value value(result);
    const char* c_value = *value;

    char* ret = tobyCall(c_key, c_value);
    result = String::NewFromUtf8(isolate, ret, NewStringType::kNormal).ToLocalChecked();
  }

  args.GetReturnValue().Set(result);
}

static void atExitCB(void* arg) {
  Isolate* isolate = static_cast<Isolate*>(arg);
  HandleScope handle_scope(isolate);
  Local<Object> obj = Object::New(isolate);
  assert(!obj.IsEmpty());  // Assert VM is still alive.
  assert(obj->IsObject());
}

void init(Local<Object> exports) {
  AtExit(atExitCB, exports->GetIsolate());

  NODE_SET_METHOD(exports, "hello", HelloMethod);
  NODE_SET_METHOD(exports, "add", AddMethod);
  NODE_SET_METHOD(exports, "callback", CallbackMethod);
  NODE_SET_METHOD(exports, "compile", CompileMethod);
  NODE_SET_METHOD(exports, "globalGet", GlobalGetMethod);

  NODE_SET_METHOD(exports, "call", CallMethod);

  tobyOnLoad(exports->GetIsolate());
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(toby, init)
}  // namespace


extern "C" void toby(const char* nodePath) {
  int (*Start)(int, char **);
  void *handle = dlopen(nodePath, RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  int _argc = 2;
  char* _argv[2];
  _argv[0] = (char*)"a.out";
  _argv[1] = (char*)"app.js";

  node::Start(_argc, _argv);
}
