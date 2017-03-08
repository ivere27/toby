# Embed Node.js into C ABI languages(C++ or Pascal/Delphi or Etc..)

## Example - Simple
```c++
#include <unistd.h>

extern "C" void  tobyInit(const char*, const char*, void*, void*, char*);

int main(int argc, char *argv[]) {
  const char* userScript = "console.log(process.version)";
  tobyInit(nullptr, userScript, nullptr, nullptr, nullptr);

  usleep(1000*1000);
  return 0;
}

// v6.10.0
```

## Example - Full
```c++
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
```
#### app.js
```javascript
'use strict'

// print toby.version
console.log(`node :: toby.version = ${toby.version}`);

// assgined from example.cpp
console.log(`node :: _v = ${_v}`);

var num = 42;
var foo = 'foo';

toby.on('test', function(x){
  console.log(`node :: toby.on(test) = ${x}`);
});

var result = toby.hostCall('dory', {num, foo});
console.log(`node :: toby.hostCall() = ${result}`);

// exit after 2 secs
(function(){setTimeout(function(){
	process.exitCode = 42;
},2000)})();
```
#### output
```
** topyOnLoad : 0x7f5ce4001f10
** tobyJSCompile : undefined
** tobyJSCall : ":)"
node :: toby.version = 0.1.3
node :: _v = 43
** from javascript. name = dory , value = {"num":42,"foo":"foo"}
node :: toby.hostCall() = hi there
node :: toby.on(test) = 0
node :: toby.on(test) = 1
** tobyOnUnload : 0x7f5ce4001f10 exitCode : 42

```



## BUILD
### build node.js v6.10.0 LTS
```
$ git clone https://github.com/nodejs/node.git
$ cd node && git checkout v6.10.0
$ ./configure --shared
$ make

# or ./configure --shared --debug
# for gdb/lldb debugging
```
###### copy node/out/Release/obj.target/libnode.so.48

## example - Embed Node.js into C++
#### linux
```
clang++ toby.cpp -c -o toby.o --std=c++11 \
-I../node/deps/v8/include/ -I../node/src/ -g \
&& clang++ example.cpp -o example --std=c++11 \
./libnode.so.48 toby.o -Wl,-rpath=. -ldl -lpthread -g \
&& ./example
```

#### mac
```
clang++ toby.cpp -c -o toby.o --std=c++11 -fPIC \
-I../node/deps/v8/include/ \
-I../node/deps/uv/include/ -I../node/src/ -g \
&& clang++ example.cpp -o example --std=c++11 \
./libnode.48.dylib toby.o \
-ldl -lpthread -g \
&& install_name_tool -change /usr/local/lib/libnode.48.dylib libnode.48.dylib example \
&& ./example
```

### win x86 in win10 (vc++ 2015)
```
vcbuild.bat dll x86
```
```
# run 'Visual C++ 2015 x86 Native Build Tools Command Prompt'
cl toby.cpp /c /MD /EHsc  -I../node/deps/v8/include -I../node/deps/uv/include  -I../node/src
cl example.cpp /c /MD /EHsc
link example.obj toby.obj /LIBPATH:../node/Release /LIBPATH:../node/build/Release/lib node.lib v8_libplatform.lib v8_libbase.lib WINMM.LIB


# debug
cl toby.cpp /c /MDd /DEBUG /EHsc  -I../node/deps/v8/include -I../node/deps/uv/include  -I../node/src && cl example.cpp /c /MDd /D /EHsc && link example.obj toby.obj /LIBPATH:../node/Debug /LIBPATH:../node/build/Debug/lib node.lib v8_libplatform.lib v8_libbase.lib WINMM.LIB

```

# Sister Projects
[node-pascal](https://github.com/ivere27/node-pascal) : Embed Node.js into Free Pascal  
[node-delphi](https://github.com/ivere27/node-delphi) : Embed Node.js into Delphi
