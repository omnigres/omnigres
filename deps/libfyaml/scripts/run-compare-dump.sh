#!/bin/bash
# VALGRIND=""
VALGRIND="valgrind"
BN=`basename $1`
ON="output/$BN"
mkdir -p output
a="$ON.libyaml.dump"
b="$ON.libfyaml.dump"
./src/libfyaml-parser -mlibyaml-dump "$1" >"$a"
./src/libfyaml-parser -mdump "$1" >"$b"
diff -au "$a" "$b" | tee "$ON.dump.diff"
./src/libfyaml-parser -mscan -d0 "$1" >"$ON.dump.log" 2>&1
