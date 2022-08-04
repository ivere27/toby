#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_ONE_SECOND Sleep(1000);
#else
#include <unistd.h>
#define SLEEP_ONE_SECOND usleep(1000*1000);
#endif

#include "toby.h"

using namespace std;
using namespace toby;

void tobyOnLoad(void* isolate, void* data) {
  tobyHostOn("exit", [](const char* name, int argc, char** argv, void* data){
    printf("tobyHostOn %s - argc : %d\n", name, argc);
    for(int i = 0; i<argc;i++)
      printf("tobyHostOn %s - argv[%d] = %s\n", name, i, argv[i]);
  });

  cout << "\e[32m" << "** topyOnLoad : " << isolate << endl;

  // custom source
  const char* source = "function _f(x) {"
                       "  return x ? x : ':)';"
                       "};"
                       "var _v = 43;";

  char buf[1024];
  int r = tobyJSCompile(source, buf, sizeof(buf));
  if (r < 0)
    cout << "** tobyJSCompile error - code : " << r << " , ret : " << buf << endl;
  else
    cout << "** tobyJSCompile : " << buf << endl;

  r = tobyJSCall("_f", "", buf, sizeof(buf));
  if (r < 0)
    cout << "** tobyJSCall error - code : " << r << " , ret : " << buf << endl;
  else
    cout << "** tobyJSCall : " << buf;

  cout << "\e[0m" << endl << flush;
}

void tobyOnUnload(void* isolate, int exitCode, void* data) {
  cout << "\e[31m" << "** tobyOnUnload : " << isolate;
  cout << " exitCode : " << exitCode << endl;
  cout << "\e[0m" << endl << flush;

  _exit(exitCode);
}

char* tobyHostCall(const char* name, const char* value, void* data) {
  cout << "\e[93m" << "** from javascript. name = " << name;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;

  char* buf = new char[10];
  strcpy(buf, "hi there");
  return buf;
}

void _toby(const char* processName, const char* userScript) {
  // toby(processName, userScript, onloadCB, onunloadCB, hostCallCB)
  tobyInit(processName,
           userScript,
           tobyOnLoad,
           tobyOnUnload,
           tobyHostCall,
           nullptr);
}

int main(int argc, char *argv[]) {
  const char* userScript = "require('./app.js');";

  // start toby in a thread
  std::thread n(_toby,
                argv[0],
                userScript);
  n.detach();

  // dummy loop in host
  static int i = 0;
  while(true) {
    SLEEP_ONE_SECOND;
    tobyJSEmit("test", to_string(i++).c_str());
  }

  return 0;
}
