#!/bin/sh
set -e

list_hdr_and_srcs() {
    find ./src -type f | grep .cpp$ || true
    find ./include -type f | grep .hpp$ || true
    find ./examples -type f | grep .hpp$ || true
    find ./examples -type f | grep .cpp$ || true
}

fmt_all_cpp() {
    for src in $(list_hdr_and_srcs); do
        echo "clang-format -i $src"
        clang-format -i $src
    done
}

fmt_all_cpp
