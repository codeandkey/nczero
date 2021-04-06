#!/bin/sh
# builds nczero

cd $(dirname $0)

CMAKEFLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ "$1" = "debug" ]; then
    FLAGS="$FLAGS -DCMAKE_BUILD_TYPE=Debug"
else
    FLAGS="$FLAGS -DCMAKE_BUILD_TYPE=Release"
fi

mkdir build
cd build
cmake $CMAKEFLAGS ..
make
