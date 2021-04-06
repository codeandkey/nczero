#!/bin/sh
# builds nczero

cd $(dirname $0)

CMAKEFLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ "$1" = "test" ]; then
    FLAGS="$FLAGS -DBUILD_TESTS"
fi

if [ "$1" = "debug" ]; then
    FLAGS="$FLAGS -DDEBUG"
fi

mkdir -p build
cd build
cmake $CMAKEFLAGS ..
make
