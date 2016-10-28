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
g++ example.cpp --std=c++11 \
-I../node/deps/v8/include/ \
-I../node/src/ -ldl -g ./libnode.so.51 -lpthread \
&& LD_LIBRARY_PATH=. ./a.out
```

mac
```
g++ example.cpp --std=c++11 \
-I../node/deps/v8/include/ \
-I../node/src/ -ldl -g ./libnode.51.dylib -lpthread \
&& DYLD_LIBRARY_PATH=. ./a.out
```