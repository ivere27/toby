#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <cstring>

using namespace std;

extern "C" void toby(const char* nodePath);
extern "C" char* tobyJSCompile(void* isolate, const char* source);
extern "C" char* tobyJSCall(void* isolate, const char* name, const char* value);

extern "C" void tobyOnLoad(void* isolate) {
  cout << "\e[32m" << "** topyOnLoad : " << isolate << endl;

  // custom source
  const char* source = "function __c(y) {"
                       "  this.x = 42;"
                       "  this.y = y;"
                       "  return y ? y : \":)\";"
                       "};"
                       "var __val = 43;"
                       "__c(__val);";

  char* data;
  data = tobyJSCompile(isolate, source);
  if (data != NULL) {
    cout << "** tobyJSCompile : " << data << endl;
    free(data);
  }

  data = tobyJSCall(isolate, "__c", "");
  if (data != NULL) {
    cout << "** tobyJSCall : " << data;
    free(data);
  }

  cout << "\e[0m" << endl << flush;
}

extern "C" char* tobyHostCall(void* isolate, const char* name, const char* value) {
  cout << "\e[93m" << "** from javascript. name = " << name;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;

  char* data = new char[10];
  strcpy(data, "hi there");
  return data;
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
