#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

typedef void  (*TobyOnloadCB)(void* isolate);
typedef char* (*TobyHostcallCB)(const char* name, const char* value);

extern "C" void  tobyInit(const char* processName,
                          const char* userScript,
                          TobyOnloadCB,
                          TobyHostcallCB);
extern "C" char* tobyJSCompile(const char* source);
extern "C" char* tobyJSCall(const char* name, const char* value);
extern "C" bool  tobyJSEmit(const char* name, const char* value);


void tobyOnLoad(void* isolate) {
  cout << "\e[32m" << "** topyOnLoad : " << isolate << endl;

  // custom source
  const char* source = "function _f(x) {"
                       "  return x ? x : ':)';"
                       "};"
                       "var _v = 43;";

  char* data;
  data = tobyJSCompile(source);
  if (data != NULL) {
    cout << "** tobyJSCompile : " << data << endl;
    free(data);
  }

  data = tobyJSCall("_f", "");
  if (data != NULL) {
    cout << "** tobyJSCall : " << data;
    free(data);
  }

  cout << "\e[0m" << endl << flush;
}

char* tobyHostCall(const char* name, const char* value) {
  cout << "\e[93m" << "** from javascript. name = " << name;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;

  char* data = new char[10];
  strcpy(data, "hi there");
  return data;
}


int main(int argc, char *argv[]) {
  const char* userScript = "require('./app.js');";

  // toby(processName, userScript, onloadCB, hostCallCB)
  tobyInit(argv[0],
           userScript,
           tobyOnLoad,
           tobyHostCall);

  // dummy loop in host
  static int i = 0;
  while(true) {
    usleep(1000*1000);
    tobyJSEmit("test", to_string(i++).c_str());
  }

  return 0;
}
