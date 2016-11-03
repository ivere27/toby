#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <functional>

using namespace std;


extern "C" void toby(const char* nodePath);
extern "C" void tobyOnLoad(void* isolate) {
  cout << "\e[32m" << "** topyOnLoad " << isolate << "\e[0m" << endl << flush;
}
extern "C" char* tobyCall(const char* key, const char* value) {
  cout << "\e[93m" << "** from javascript. key = " << key;
  cout << " , value = " << value << "\e[0m";
  cout << endl << flush;
  return (char*)"from example.cpp";
}

int main(int argc, char *argv[]) {
  std::thread n(toby, "./libnode.so.51");
  n.detach();



  // dummy loop
  while(true) {
    usleep(1000*1000);
    //printf("main\n");
  }

  return 0;
}
