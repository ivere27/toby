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
clang++ soy.cpp -c -o soy.o \
--std=c++11 -fPIC \
-I../node/deps/v8/include/ \
-I../node/src/ -g \
&& clang++ example.cpp -o example \
--std=c++11 \
-I../node/deps/v8/include/ \
-I../node/src/ -g ./libnode.so.51 soy.o \
-Wl,-rpath=. -ldl -lpthread \
&& ./example
```

mac
```
clang++ example.cpp --std=c++11 -fPIC \
-I../node/deps/v8/include/ \
-I../node/src/ -ldl -g ./libnode.51.dylib -lpthread \
&& DYLD_LIBRARY_PATH=. ./a.out
```
