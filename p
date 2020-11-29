#!/bin/sh
set -e

cfg_flags() {
    echo --trace
    echo --benchmarks
    true
}

build() {
    ./configure $(cfg_flags)
    make -j $(nproc)
}

run_all() {
    ./scripts/profile/1.sh
}

main() {
    # if [ ! -f bin/bench-all-reduce ]; then
    #     build
    # fi
    build
    run_all
}

main
