#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <cstring>

#include <map>
#include <vector>

#include "libplatform/libplatform.h"
#include "uv.h"
#include "node.h"

extern "C" void tobyOnLoad(void* isolate);
extern "C" char* tobyHostCall(const char* key, const char* value);

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

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  virtual void* Allocate(size_t length) {
    void* data = AllocateUninitialized(length);
    return data == NULL ? data : memset(data, 0, length);
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};

static uv_loop_t* loop;
static Isolate* __isolate;

// FIXME : vardic arguments? multiple listeners?
using Callback = std::map<std::string, Persistent<Function>>;
static Callback eventListeners;

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

extern "C" char* tobyJSCompile(const char* source) {
  Isolate* isolate = static_cast<Isolate*>(__isolate);
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

extern "C" char* tobyJSCall(const char* name, const char* value) {
  Isolate* isolate = static_cast<Isolate*>(__isolate);

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

    char* ret = tobyHostCall(c_key, c_value);
    result = String::NewFromUtf8(isolate, ret, NewStringType::kNormal).ToLocalChecked();
  }

  args.GetReturnValue().Set(result);
}


// /node/test/addons/async-hello-world/binding.cc
struct async_req {
  uv_work_t req;
  std::string name;
  std::string value;
  v8::Isolate* isolate;
  // v8::Persistent<v8::Function> callback;
};

void DoAsync(uv_work_t* r) {
  async_req* req = reinterpret_cast<async_req*>(r->data);
  // printf("DoAsync\n");
}


void AfterAsync(uv_work_t* r, int status) {
  // FIXME : check the node.js is still alive

  // printf("AfterAsync\n");
  async_req* req = reinterpret_cast<async_req*>(r->data);

  if (eventListeners.count(req->name) == 0) {
    delete req;
    return;
  }

  v8::Isolate* isolate = req->isolate;
  auto context = isolate->GetCurrentContext();

  v8::HandleScope scope(isolate);

  std::vector<Local<Value>> argv;
  Local<Value> argument = String::NewFromUtf8(isolate, req->value.c_str());  //value.c_str()
  argv.push_back(argument);


  v8::TryCatch try_catch(isolate);
  Local<Value> result;

  Local<Function> callback = Local<Function>::New(isolate, eventListeners[req->name]);
  result = callback->Call(context->Global(), argv.size(), argv.data());

  // // cleanup
  // req->callback.Reset();
  delete req;

  if (try_catch.HasCaught()) {
    node::FatalException(isolate, try_catch);
  }
}

extern "C" bool tobyJSEmit(const char* name, const char* value) {
  async_req* req = new async_req;
  req->req.data = req;
  req->isolate = __isolate; // FIXME : ...

  req->name = std::string(name);  // FIXME : use pointer
  req->value = std::string(value);

  uv_queue_work(loop,
                &req->req,
                DoAsync,
                AfterAsync);
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

    // FIXME : remove it in removeListener()
    //eventListeners[name].Reset();
  }
}

static void _tobyInit(Isolate* isolate) {
  // dummy event. do not end the loop.
  // FIXME : use uv_async_init!!
  const char* source = "(function(){setInterval(function(){},1000)})();";

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

  NODE_SET_METHOD(exports, "hostCall", HostCallMethod);
  NODE_SET_METHOD(exports, "on", OnMethod);

  // call the toby's internal Init()
  _tobyInit(__isolate);

  // call the host's OnLoad()
  tobyOnLoad(__isolate);
}

NODE_MODULE_CONTEXT_AWARE_BUILTIN(toby, init)
}  // namespace


static void _node(const char* nodePath, const char* processName, const char* userScript) {
  int (*Start)(int, char **);
  void *handle = dlopen(nodePath, RTLD_LAZY | RTLD_NODELETE);
  Start = (int (*)(int, char **))dlsym(handle, "Start");

  const char tobyScript[] = "const toby = process.binding('toby');";

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

  {
    loop = uv_default_loop();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = new ArrayBufferAllocator();
    static  v8::Platform* platform_;
    platform_ = v8::platform::CreateDefaultPlatform();  //v8_default_thread_pool_size = 4;
    v8::V8::InitializePlatform(platform_);
    v8::V8::Initialize();

    __isolate = Isolate::New(create_params);

    v8::Locker locker(__isolate);
    Isolate::Scope isolate_scope(__isolate);
    HandleScope handle_scope(__isolate);
    static  Local<Context> context = Context::New(__isolate);
    Context::Scope context_scope(context);

    int exec_argc;
    const char** exec_argv;
    node::Init(&_argc, const_cast<const char**>(_argv), &exec_argc, &exec_argv);

    static node::Environment* env = node::CreateEnvironment(
        __isolate, loop, context, _argc, _argv, exec_argc, exec_argv);

    node::LoadEnvironment(env);

    bool more;
    do {
      more = uv_run(loop, UV_RUN_ONCE);
      if (more == false) {
        node::EmitBeforeExit(env);

        more = uv_loop_alive(loop);
        if (uv_run(loop, UV_RUN_NOWAIT) != 0)
          more = true;
      }
    } while (more == true);
  }

  //node::Start(_argc, _argv);
}

extern "C" void toby(const char* nodePath, const char* processName, const char* userScript) {
  std::thread n(_node, nodePath, processName, userScript);
  n.detach();
}
