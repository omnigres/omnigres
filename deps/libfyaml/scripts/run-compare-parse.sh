#!/bin/bash
# VALGRIND=""
VALGRIND="valgrind"
BN=`basename $1`
ON="output/$BN"
mkdir -p output
a="$ON.libyaml.parse"
b="$ON.libfyaml.parse"
./src/libfyaml-parser -mlibyaml-parse "$1" >"$a"
./src/libfyaml-parser -mparse "$1" >"$b"
diff -u "$a" "$b" | tee "$ON.parse.diff"
./src/libfyaml-parser -mparse -d0 "$1" >"$ON.parse.log" 2>&1
