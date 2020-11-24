#!/bin/sh
set -e

build() {
    ./configure
    make
}

run_all() {
    ./scripts/bench/small.sh
    # ./scripts/bench/1.sh
    # ./scripts/vis/timeline.sh
}

main() {
    # if [ ! -f bin/bench-all-reduce ]; then
    #     build
    # fi
    build
    run_all
}

main
