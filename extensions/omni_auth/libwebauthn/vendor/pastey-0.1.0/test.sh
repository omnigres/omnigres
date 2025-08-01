#!/bin/bash

set -e  # Exit immediately if any command exits with a non-zero status

echo "========================================"
echo "Running tests with pastey crate..."
echo "========================================"
cargo test

echo "========================================"
echo "Running tests with pastey-test-suite crate..."
echo "========================================"
cd pastey-test-suite
cargo test
cd ../

cd paste-compat
./test.sh
cd ../


