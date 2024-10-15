#!/bin/bash

set -e

mkdir -p tests/build
cd tests/build
cmake ..
cmake --build .

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo "Testing ./gen ..."
    ./gen

    echo "Testing ./stmt ..."
    ./stmt
fi
