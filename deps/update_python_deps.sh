#! /usr/bin/env bash

set -e

rm -rf python
mkdir -p python

python3 -m pip download --no-binary all -r requirements.txt -r ../docs/requirements.txt -r ../tools/requirements.txt -d python
