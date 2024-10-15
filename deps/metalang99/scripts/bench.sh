#!/bin/bash

set -e

bench() {
    echo $1
    time gcc bench/$1 -ftrack-macro-expansion=0 -Iinclude -E -P >/dev/null
    echo ""
}

bench "compare_25_items.c"
bench "list_of_63_items.c"
bench "100_v.c"
bench "100_call.c"
bench "many_call_in_arg_pos.c"
bench "filter_map.c"
