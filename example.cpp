#include <cassert>
#include <iostream>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <thread>

extern "C" void _node(const char* nodePath);

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
