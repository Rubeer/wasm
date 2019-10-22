
#!/bin/bash

# Set DIR to the directory this script is located at.
# This ensures that this script will work when ran from a different working directory.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p $DIR/bin

pushd $DIR/bin > /dev/null

# Translate C++ to LLVM bytecode
clang -c -emit-llvm -Wall -Wno-writable-strings -Ofast --target=wasm32 -nostdlib -std=c++11 -fvisibility=hidden -fno-exceptions -fno-rtti -fno-threadsafe-statics $DIR/main.cpp

if [[ $? -eq 0 ]]
then

    # Optimize LLVM bytecode
    opt -O3 main.bc -o main.bc
    
    # Compile to WebAssembly
    llc -O3 -filetype=obj main.bc -o main.o
    
    # Link, add js function imports
    wasm-ld --no-entry main.o -o main.wasm --strip-all -allow-undefined-file $DIR/wasm.syms --import-memory --export-dynamic
    
    cp main.wasm /usr/share/nginx/html/
    cp $DIR/index.html /usr/share/nginx/html/
fi

popd > /dev/null
