#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    echo -q
    echo -logdir logs/example-1
    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    env KUNGFU_CONFIG_LOG_LEVEL=debug \
        kungfu-run $(kungfu_run_flags $np) $@
}

mpi_run() {
    local np=$1
    shift
    mpirun -np $np $@
}

main() {
    local np=4
    kungfu_run $np ./bin/example-1
    # mpi_run $np ./bin/example-1
}

main
