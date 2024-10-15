#!/bin/bash

set -e

mkdir -p examples/build
cd examples/build
cmake ..
cmake --build .
