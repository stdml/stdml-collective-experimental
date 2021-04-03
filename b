#!/bin/sh
set -e

. ./scripts/profile/measure.sh

# export STDML_COLLECTIVE_ENABLE_LOG=1
config_flags() {
    echo --benchmarks
    echo --quiet
}

build() {
    measure ./configure $(config_flags)
    measure make -j $(nproc)
}

run_all() {
    # ./scripts/bench/small.sh
    ./scripts/bench/1.sh
    # ./scripts/vis/timeline.sh
}

main() {
    # if [ ! -f bin/bench-all-reduce ]; then
    measure build
    # fi
    rm -fr logs
    run_all
}

main
