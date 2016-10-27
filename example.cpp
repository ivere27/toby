#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>

#include "node.h"

namespace demo {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Number;
using v8::String;
using v8::Handle;
using v8::Local;
using v8::Value;


void AddMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  static int i = 0;
  args.GetReturnValue().Set(i++);
}

void HelloMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, "world"));
}

void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "hello", HelloMethod);
  NODE_SET_METHOD(exports, "add", AddMethod);
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(soy, init)
}  // namespace demo

void _node() {
  int (*Start)(int, char **);
  void *handle = dlopen("libnode.so.51", RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  int _argc = 2;
  char* _argv[2];
  _argv[0] = "a.out";
  _argv[1] = "app.js";

  node::Start(_argc, _argv);
}


int main(int argc, char *argv[]) {
  std::thread n(_node);
  n.detach();

  // dummy loop
  while(true) {
    usleep(1000*1000);
    //printf("main\n");
  }

  return 0;
}
