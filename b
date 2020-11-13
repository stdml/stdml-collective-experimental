#!/bin/sh
set -e

build() {
    ./configure
    make
}

run_all() {
    ./scripts/bench/1.sh
    # ./scripts/vis/timeline.sh
}

main() {
    build
    run_all
}

main
