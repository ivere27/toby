# Embed Node.js into C ABI languages(C++ or Pascal or Etc..)

### build node.js v6.9.1 LTS
```
$ git checkout v6.9.1
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
"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\cl.exe" toby.cpp /c -I"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include" -I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.10240.0\ucrt" -I"C:\Program Files (x86)\Windows Kits\8.1\Include\um" -I"C:\Program Files (x86)\Windows Kits\8.1\Include\shared" -I../node/deps/v8/include -I../node/deps/uv/include  -I../node/src
```

# Sister Projects
[node-pascal](https://github.com/ivere27/node-pascal) : Embed Node.js into Free Pascal
