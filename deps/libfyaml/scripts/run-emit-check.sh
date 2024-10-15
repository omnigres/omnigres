#!/bin/bash
BN=`basename $1`
ON="output/$BN"
mkdir -p output
a="$ON.1.dump"
b="$ON.2.dump"
./src/fy-tool --testsuite "$1" >"$a"
./src/fy-tool --dump "$1" | ./src/fy-tool --testsuite - >"$b"
diff -au "$a" "$b" | tee "$ON.12.diff"
if [ ! -s "${ON}.12.diff" ] ; then
	rm -f "${ON}.1.dump" "${ON}.2.dump" "${ON}.12.diff"
fi
