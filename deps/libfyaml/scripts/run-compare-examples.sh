#!/bin/bash

for i in $*; do
	echo "checking: $i"
	./run-compare-scan.sh $i
	./run-compare-parse.sh $i
	echo
done
