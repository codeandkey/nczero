#!/bin/sh
# runs tests

cd $(dirname $0)

./compile.sh test

cpus=$(nproc --all)

echo "Building with $cpus threads."

cd build
ctest -j $cpus

cd ..
gcovr --exclude '.*google.*'
gcovr --exclude '.*google.*' --coveralls result.json
