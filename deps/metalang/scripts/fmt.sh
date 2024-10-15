#!/bin/bash

find include tests examples bench \
    \( -path examples/build -o -path tests/build \) -prune -false -o \
    \( -iname "*.h" \) -or \( -iname "*.c" \) | xargs clang-format -i
