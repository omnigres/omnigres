#!/bin/bash

./run-clang-format/run-clang-format.py \
    --exclude examples/build \
    --exclude tests/build \
    -r include tests examples bench
