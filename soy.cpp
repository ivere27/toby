#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>

#include "node.h"

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

void AddMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  args.GetReturnValue().Set(add());
}

void HelloMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
}

void CallbackMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();
  v8::Local<v8::Value> result;

  if (args[0]->IsFunction()) {
    std::vector<v8::Local<v8::Value>> argv;
    v8::Local<v8::Value> argument = Number::New(isolate, add());
    argv.push_back(argument);

    auto method = args[0].As<v8::Function>();
    result = node::MakeCallback(isolate,
      isolate->GetCurrentContext()->Global(), method,
      argv.size(), argv.data());
  }
  else {
    result = Undefined(isolate);
  }

  args.GetReturnValue().Set(result);
}


void CompileMethod(const v8::FunctionCallbackInfo<v8::Value>& args) {
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

static Local<Value> GetValue(v8::Isolate* isolate, Local<Context> context,
                             Local<v8::Object> object, const char* property) {
  Local<String> v8_str =
      String::NewFromUtf8(isolate, property, NewStringType::kNormal)
          .ToLocalChecked();
  return object->Get(context, v8_str).ToLocalChecked();
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

static void atExitCB(void* arg) {
  Isolate* isolate = static_cast<Isolate*>(arg);
  HandleScope handle_scope(isolate);
  Local<Object> obj = Object::New(isolate);
  assert(!obj.IsEmpty());  // Assert VM is still alive.
  assert(obj->IsObject());
  printf("bye~\n");
}

void init(Local<Object> exports) {
  AtExit(atExitCB, exports->GetIsolate());
  NODE_SET_METHOD(exports, "hello", HelloMethod);
  NODE_SET_METHOD(exports, "add", AddMethod);
  NODE_SET_METHOD(exports, "callback", CallbackMethod);
  NODE_SET_METHOD(exports, "compile", CompileMethod);
  NODE_SET_METHOD(exports, "globalGet", GlobalGetMethod);
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(soy, init)
}  // namespace


extern "C" void _node(const char* nodePath) {
  int (*Start)(int, char **);
  void *handle = dlopen(nodePath, RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  int _argc = 2;
  char* _argv[2];
  _argv[0] = (char*)"a.out";
  _argv[1] = (char*)"app.js";

  node::Start(_argc, _argv);
}
