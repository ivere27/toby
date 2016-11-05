#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <functional>

using namespace std;


extern "C" void toby(const char* nodePath);
extern "C" bool tobyJSCompile(void* isolate, const char* source);
extern "C" bool tobyJSCall(void* arg, const char* name, const char* value, char* r);

extern "C" void tobyOnLoad(void* isolate) {
  cout << "\e[32m" << "** topyOnLoad : " << isolate << endl;

  // test source
  const char* source = "function __c(y) {"
                       "  this.x = 42;"
                       "  this.y = y;"
                       "  return \":)\";"
                       "};"
                       "var __val = 43;";

  cout << "** tobyJSCompile : " << tobyJSCompile(isolate, source) << endl;

  char* ret = new char[1024];
  tobyJSCall(isolate, "__c", "2", ret);
  cout << "** tobyJSCall : " << ret;
  cout << "\e[0m" << endl << flush;
}

extern "C" char* tobyHostCall(const char* name, const char* value) {
  cout << "\e[93m" << "** from javascript. name = " << name;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;
  return (char*)"hi there";
}


int main(int argc, char *argv[]) {
  toby("./libnode.so.51");

  // dummy loop
  while(true) {
    usleep(1000*1000);
    //printf("main\n");
  }

  return 0;
}
