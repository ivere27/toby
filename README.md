# Embed Node.js into C++

build node.js
```
./configure --shared
make

```
#### copy node/out/Release/obj.target/libnode.so.51

## example
linux
```
clang++ toby.cpp -c -o toby.o \
--std=c++11 -fPIC \
-I../node/deps/v8/include/ \
-I../node/src/ -g \
&& clang++ example.cpp -o example --std=c++11 \
./libnode.so.51 toby.o \
-Wl,-rpath=. -ldl -lpthread \
-g \
&& ./example
```

mac
```
clang++ toby.cpp -c -o toby.o \
--std=c++11 -fPIC \
-I../node/deps/v8/include/ \
-I../node/src/ -g \
&& clang++ example.cpp -o example --std=c++11 \
./libnode.51.dylib toby.o \
-ldl -lpthread \
-g \
&& DYLD_LIBRARY_PATH=. ./example
```
