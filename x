#!/bin/sh
set -e

build() {
    ./configure
    make
}

run_all() {
    # ./scripts/run/example-1.sh
    ./scripts/run/example-mpi.sh
    # ./scripts/bench/1.sh
    # ./scripts/bench/baseline.sh
    # ./scripts/vis/timeline.sh
}

main() {
    build
    rm -fr logs
    run_all
}

# main
# ./scripts/vis/timeline.sh

build
./scripts/run/example-mpi.sh
