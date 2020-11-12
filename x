#!/bin/sh
set -e

build() {
    ./configure
    make
}

run_all() {
    ./scripts/run/example-1.sh
    ./scripts/bench/1.sh
    # ./scripts/bench/baseline.sh
}

main() {
    build
    rm -fr logs
    run_all
}

main
