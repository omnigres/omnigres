#! /usr/bin/env bash
# Run GCC preprocessor and delete empty lines
: ${CPP:=cpp}
"${CPP}" -DURI_DOXYGEN -DURI_NO_UNICODE -C -I ../include "$1" | sed -e '/^$/d' -e 's/COMMENT_HACK//g'
