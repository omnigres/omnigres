#!/usr/bin/env bash

git clone https://github.com/cppgres/cppgres -b amalgamated --single-branch --depth=1
cp cppgres/cppgres.hpp deps
rm -rf cppgres
