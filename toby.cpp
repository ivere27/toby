#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <cstring>

#include <map>
#include <queue>
#include <vector>
#include "node.h"

extern "C" void tobyOnLoad(void* isolate);
extern "C" char* tobyHostCall(void* isolate, const char* key, const char* value);

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
using v8::Handle;
using v8::Persistent;

// FIXME : vardic arguments? multiple listeners?
using Callback = std::map<std::string, Persistent<Function>>;
static Callback eventListeners;
static std::queue<std::tuple<std::string, std::string>> eventQueue;

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

static void CallbackMethod(const FunctionCallbackInfo<Value>& args) {
  auto isolate = args.GetIsolate();
  Local<Value> result;

  if (args[0]->IsFunction()) {
    std::vector<Local<Value>> argv;
    Local<Value> argument = Number::New(isolate, 0);
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

extern "C" char* tobyJSCompile(void* arg, const char* source) {
  Isolate* isolate = static_cast<Isolate*>(arg);
  Local<Value> result;

  HandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);

  Local<Context> context(isolate->GetCurrentContext());

  Local<String> script = String::NewFromUtf8(isolate, source);
  Local<Script> compiled_script;
  if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
    String::Utf8Value error(try_catch.Exception());
    return NULL;
  }

  if (!compiled_script->Run(context).ToLocal(&result)) {
    String::Utf8Value error(try_catch.Exception());
    return NULL;
  }

  result = Stringify(isolate, context, result);
  v8::String::Utf8Value ret(result);

  char* data = new char[ret.length()];
  strcpy(data, *ret);
  return data;
}

extern "C" char* tobyJSCall(void* _isolate, const char* name, const char* value) {
  Isolate* isolate = static_cast<Isolate*>(_isolate);

  auto context = isolate->GetCurrentContext();
  auto global = context->Global();

  Local<Value> result;
  auto func = GetValue(isolate, context, global, name);

  if (func->IsFunction()) {
    std::vector<Local<Value>> argv;
    Local<Value> argument = String::NewFromUtf8(isolate, value);
    argv.push_back(argument);

    auto method = func.As<Function>();
    result = node::MakeCallback(isolate,
      isolate->GetCurrentContext()->Global(), method,
      argv.size(), argv.data());
  }
  else {
    result = Undefined(isolate);
    return NULL;
  }

  result = Stringify(isolate, context, result);
  v8::String::Utf8Value ret(result);

  char* data = new char[ret.length()];
  strcpy(data, *ret);
  return data;
}

static void HostCallMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Value> result;

  auto context = isolate->GetCurrentContext();
  auto global = context->Global();

  {
    // FIXME : better way to serialize/deserialize?
    result = Stringify(isolate, context, args[1]);

    v8::String::Utf8Value key(args[0]);
    const char* c_key = *key;

    v8::String::Utf8Value value(result);
    const char* c_value = *value;

    char* ret = tobyHostCall(isolate, c_key, c_value);
    result = String::NewFromUtf8(isolate, ret, NewStringType::kNormal).ToLocalChecked();
  }

  args.GetReturnValue().Set(result);
}

extern "C" bool tobyJSEmit(const char* name, const char* value) {
  eventQueue.push(std::make_tuple(std::string(name), std::string(value)));
  return true;
}

static void OnMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  //Local<Value> result = Undefined(isolate);

  auto context = isolate->GetCurrentContext();
  auto global = context->Global();

  if (args[1]->IsFunction()) {
    v8::String::Utf8Value name(args[0]);

    Local<Function> callback = Local<Function>::Cast(args[1]);
    eventListeners[std::string(*name)].Reset(isolate, callback);
  }
}

// FIXME : temporal eventloop. need to use the libuv's async
// node/test/addons/async-hello-world/binding.cc
static void PollingMethod(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  HandleScope scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto global = context->Global();

  while(eventQueue.size() > 0) {
    auto e = eventQueue.front();
    eventQueue.pop();
    auto name = std::get<0>(e);
    auto value = std::get<1>(e);

    if (eventListeners.count(name)) {
      Local<Value> result;
      std::vector<Local<Value>> argv;
      Local<Value> argument = String::NewFromUtf8(isolate, value.c_str());
      argv.push_back(argument);

      Local<Function> callback = Local<Function>::New(isolate, eventListeners[name]);
      result = callback->Call(global, argv.size(), argv.data());

      // FIXME : remove it in removeListener()
      //eventListeners[name].Reset();
    }
  }
}

static void _tobyInit(Isolate* isolate) {
  const char* source = "(function(){})();";

  Local<Value> result;
  HandleScope handle_scope(isolate);
  TryCatch try_catch(isolate);

  Local<Context> context(isolate->GetCurrentContext());

  Local<String> script = String::NewFromUtf8(isolate, source);
  Local<Script> compiled_script;
  if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
    String::Utf8Value error(try_catch.Exception());
    // printf("%s", *error);
    return;
  }

  if (!compiled_script->Run(context).ToLocal(&result)) {
    String::Utf8Value error(try_catch.Exception());
    // printf("%s", *error);
    return;
  }

  // result = Stringify(isolate, context, result);
  // v8::String::Utf8Value ret(result);
}

static void atExitCB(void* arg) {
  Isolate* isolate = static_cast<Isolate*>(arg);
  HandleScope handle_scope(isolate);
  Local<Object> obj = Object::New(isolate);
  assert(!obj.IsEmpty());  // Assert VM is still alive.
  assert(obj->IsObject());
}

static void init(Local<Object> exports) {
  AtExit(atExitCB, exports->GetIsolate());

  NODE_SET_METHOD(exports, "callback", CallbackMethod);
  NODE_SET_METHOD(exports, "hostCall", HostCallMethod);
  NODE_SET_METHOD(exports, "_polling", PollingMethod);
  NODE_SET_METHOD(exports, "on", OnMethod);

  // call the toby's internal Init()
  _tobyInit(exports->GetIsolate());

  // call the host's OnLoad()
  tobyOnLoad(exports->GetIsolate());
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(toby, init)
}  // namespace


static void _node(const char* nodePath, const char* processName, const char* userScript) {
  int (*Start)(int, char **);
  void *handle = dlopen(nodePath, RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  char* tobyScript = (char*)"const toby = process.binding('toby');"
                            "setInterval(function(){toby._polling();},100);";
  std::string initScript;
  initScript += tobyScript;
  initScript += '\n';
  initScript += userScript;
  initScript += '\n';

  // argv memory should be adjacent.
  // libuv/src/unix/proctitle.c
  int _argc = 3;
  char* _argv[_argc];

  char* nodeOptions = (char*)"-e";

  int size = 0;
  size += strlen(processName) + 1;
  size += strlen(nodeOptions) + 1;
  size += initScript.length() + 1;

  char* buf = new char[size];
  int i = 0;

  _argv[0] = buf+i;
  strncpy(buf+i, processName, strlen(processName));
  i += strlen(processName);
  buf[++i] = '\0';

  _argv[1] = buf+i;
  strncpy(buf+i, nodeOptions, strlen(nodeOptions));
  i += strlen(nodeOptions);
  buf[++i] = '\0';

  _argv[2] = buf+i;
  strcpy(buf+i, initScript.c_str());

  node::Start(_argc, _argv);
}

extern "C" void toby(const char* nodePath, const char* processName, const char* userScript) {
  std::thread n(_node, nodePath, processName, userScript);
  n.detach();
}
