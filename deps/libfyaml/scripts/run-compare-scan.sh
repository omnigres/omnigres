#!/bin/bash
# VALGRIND=""
VALGRIND="valgrind"
BN=`basename $1`
ON="output/$BN"
mkdir -p output
a="$ON.libyaml.scan"
b="$ON.libfyaml.scan"
./src/libfyaml-parser -mlibyaml-scan "$1" >"$a"
./src/libfyaml-parser -mscan "$1" >"$b"
diff -u "$a" "$b" | tee "$ON.scan.diff"
./src/libfyaml-parser -mscan -d0 "$1" >"$ON.scan.log" 2>&1
