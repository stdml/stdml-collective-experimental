#!/bin/sh
set -e

cfg_flags() {
    echo --mpi
    echo --go-runtime
    echo --with-go-runtime=$HOME/local
}

build() {
    ./configure $(cfg_flags)
    make -j 8
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
# ./scripts/run/example-mpi.sh
# ./scripts/bench/mpi.sh
export STDML_COLLECTIVE_USE_GO_RUNTIME=1
./scripts/bench/small.sh
