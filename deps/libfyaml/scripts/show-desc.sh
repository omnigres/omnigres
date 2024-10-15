#!/bin/bash
for i in output/*.desc; do echo -n "$i: "; cat $i;  done
