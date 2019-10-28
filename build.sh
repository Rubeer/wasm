#!/bin/bash

# Set DIR to the directory this script is located at.
# This ensures that this script will work when ran from a different working directory.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p $DIR/bin

pushd $DIR/bin > /dev/null

# We turn on ALL warnings and selectively turn off the ones we don't like.
warning_flags="-Weverything -Wno-missing-prototypes -Wno-old-style-cast -Wno-writable-strings -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-extra-semi-stmt -Wno-gnu-zero-variadic-macro-arguments -Wno-zero-as-null-pointer-constant -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-missing-braces -Wno-unused-template -Wno-unused-function -Wno-padded -Wno-cast-qual"
# Turn off dumb "features" (maybe redundant when compiling bare wasm)
make_cpp_dumb="-fno-exceptions -fno-rtti -fno-threadsafe-statics -fwrapv"

# Translate C++ to LLVM bytecode
clang -c -emit-llvm -O2 -ffast-math --target=wasm32 -nostdlib -std=c++11 -fvisibility=hidden $make_cpp_dumb $warning_flags $DIR/main.cpp

if [[ $? -eq 0 ]]
then

    # Optimize LLVM bytecode. Maybe redundant, the llc flag should do it too?
    opt -O3 main.bc -o main.bc
    
    # Compile to WebAssembly
    llc -O3 -filetype=obj main.bc -o main.o
    
    # Link, add js function imports
    wasm-ld --no-entry main.o -o main.wasm --strip-all -allow-undefined-file $DIR/js_imported_functions.syms --export-dynamic --import-memory --stack-first

    # Strip out static variable bloat (zeroes)
    wasm-opt -O2 main.wasm -o main.wasm
    
    cp main.wasm /usr/share/nginx/html/
    cp $DIR/index.html /usr/share/nginx/html/
fi

popd > /dev/null
