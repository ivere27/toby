#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>
#include <functional>

using namespace std;


extern "C" void _node(const char* nodePath);
extern "C" char* _onSetValue(const char* key, const char* value) {
  cout << "** from javascript. key = " << key << " , value = " << value;
  cout << endl << flush;
  return (char*)"from example.cpp";
}

int main(int argc, char *argv[]) {
  std::thread n(_node, "./libnode.so.51");
  n.detach();



  // dummy loop
  while(true) {
    usleep(1000*1000);
    //printf("main\n");
  }

  return 0;
}
