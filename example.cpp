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

void tobyOnLoad(void* isolate) {
  tobyHostOn("exit", [](int argc, char** argv){
    printf("tobyHostOn - argc : %d\n", argc);
    for(int i = 0; i<argc;i++)
      printf("tobyHostOn - argv[%d] = %s\n",i, argv[i]);
  });

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

  _exit(exitCode);
}

char* tobyHostCall(const char* name, const char* value) {
  cout << "\e[93m" << "** from javascript. name = " << name;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;

  char* data = new char[10];
  strcpy(data, "hi there");
  return data;
}

void _toby(const char* processName, const char* userScript) {
  // toby(processName, userScript, onloadCB, onunloadCB, hostCallCB)
  tobyInit(processName,
           userScript,
           tobyOnLoad,
           tobyOnUnload,
           tobyHostCall);
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
