#!/bin/bash

# Set DIR to the directory this script is located at.
# This ensures that this script will work when ran from a different working directory.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cp $DIR/index.html /usr/share/nginx/html/
cp $DIR/*.js /usr/share/nginx/html/

mkdir -p $DIR/bin
pushd $DIR/bin > /dev/null

# We turn on ALL warnings and selectively turn off the ones we don't like.
warning_flags="-Weverything -Wno-missing-prototypes -Wno-old-style-cast -Wno-writable-strings -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-extra-semi-stmt -Wno-gnu-zero-variadic-macro-arguments -Wno-zero-as-null-pointer-constant -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-missing-braces -Wno-unused-template -Wno-unused-function -Wno-padded -Wno-cast-qual -Wno-unused-const-variable -Wno-cast-align -Wno-char-subscripts -Wno-unused-macros -Wno-double-promotion"
make_cpp_dumb="-nostdinc++ -fno-exceptions -fno-rtti -fno-threadsafe-statics -fwrapv"

# Translate C++ to LLVM bytecode
echo clang
clang -c -emit-llvm -O2 -ffast-math -ffreestanding --target=wasm32 -nostdlib -std=c++11 -fvisibility=hidden $make_cpp_dumb $warning_flags $DIR/main.cpp

if [[ $? -eq 0 ]]
then

    # Optimize LLVM bytecode. Maybe redundant, the llc flag should do it too?
    echo opt
    opt -O3 main.bc -o main.bc
    
    # Compile to WebAssembly
    echo llc
    llc -O3 -filetype=obj main.bc -o main.o #-mattr=+bulk-memory -mattr=+atomics
    
    # Link, add js function imports
    echo wasm-ld
    wasm-ld --no-entry main.o -o main.wasm --strip-all -allow-undefined-file $DIR/js_imported_functions.syms --export-all --stack-first --initial-memory=67108864 #--max-memory=536870912 --shared-memory

    # Strip out static variable bloat (zeroes)
    echo wasm-opt
    wasm-opt -O2 main.wasm -o main.wasm #--enable-bulk-memory --enable-threads
    
    cp main.wasm /usr/share/nginx/html/
fi

popd > /dev/null
