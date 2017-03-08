#include <cstring>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_ONE_SECOND Sleep(1000);
#else
#include <unistd.h>
#define SLEEP_ONE_SECOND usleep(1000*1000);
#endif


using namespace std;

typedef void  (*TobyOnloadCB)(void* isolate);
typedef void  (*TobyOnunloadCB)(void* isolate, int exitCode);
typedef char* (*TobyHostcallCB)(const char* name, const char* value);

extern "C" void tobyInit(const char* processName,
                         const char* userScript,
                         TobyOnloadCB,
                         TobyOnunloadCB,
                         TobyHostcallCB);
extern "C" int  tobyJSCompile(const char* source, char* dest, size_t n);
extern "C" int  tobyJSCall(const char* name, const char* value, char* dest, size_t n);
extern "C" int  tobyJSEmit(const char* name, const char* value);


void tobyOnLoad(void* isolate) {
  cout << "\e[32m" << "** topyOnLoad : " << isolate << endl;

  // custom source
  const char* source = "function _f(x) {"
                       "  return x ? x : ':)';"
                       "};"
                       "var _v = 43;";

  char data[1024];
  int ret = tobyJSCompile(source, data, sizeof(data));
  if (ret < 0)
    cout << "** tobyJSCompile error - code : " << ret << " , data : " << data << endl;
  else
    cout << "** tobyJSCompile : " << data << endl;

  ret = tobyJSCall("_f", "", data, sizeof(data));
  if (ret < 0)
    cout << "** tobyJSCall error - code : " << ret << " , data : " << data << endl;
  else
    cout << "** tobyJSCall : " << data;

  cout << "\e[0m" << endl << flush;
}

void tobyOnUnload(void* isolate, int exitCode) {
  cout << "\e[31m" << "** tobyOnUnload : " << isolate;
  cout << " exitCode : " << exitCode << endl;
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

  // toby(processName, userScript, onloadCB, onunloadCB, hostCallCB)
  tobyInit(argv[0],
           userScript,
           tobyOnLoad,
           tobyOnUnload,
           tobyHostCall);

  // dummy loop in host
  static int i = 0;
  while(true) {
    SLEEP_ONE_SECOND;
    tobyJSEmit("test", to_string(i++).c_str());
  }

  return 0;
}
