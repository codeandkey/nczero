#!/bin/sh
# builds nczero

cd $(dirname $0)

CMAKEFLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ "$1" = "test" ]; then
    CMAKEFLAGS="$CMAKEFLAGS -DBUILD_TESTS=ON"
fi

if [ "$1" = "debug" ]; then
    CMAKEFLAGS="$CMAKEFLAGS -DDEBUG=ON"
fi

shift

mkdir -p build
cd build
echo $@
cmake $CMAKEFLAGS $@ --verbose ..
make
