#!/usr/bin/env bash

# Script to re-vendor the WIT files that wasi-rs uses as defined by a
# particular tag in upstream repositories.
#
# This script is executed on CI to ensure that everything is up-to-date.
set -ex

# Space-separated list of wasi proposals that are vendored here along with the
# tag that they're all vendored at.
#
# This assumes that the repositories all have the pattern:
# https://github.com/WebAssembly/wasi-$repo
# and every repository has a tag `v$tag` here. That is currently done as part
# of the WASI release process.
repos="cli clocks filesystem http io random sockets"
tag=0.2.4
dst=wit/deps

rm -rf $dst
mkdir -p $dst

for repo in $repos; do
  mkdir $dst/$repo
  curl -L https://github.com/WebAssembly/wasi-$repo/archive/refs/tags/v$tag.tar.gz | \
    tar xzf - --strip-components=2 -C $dst/$repo wasi-$repo-$tag/wit
  rm -rf $dst/$repo/deps*
done
