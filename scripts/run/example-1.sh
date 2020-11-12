#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    echo -q
    echo -logdir logs
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

logs() {
    cd logs
    mkdir -p out
    mkdir -p err
    mv *.stdout.* out
    mv *.stderr.* err
}

main() {
    local np=4
    rm -fr logs
    kungfu_run $np ./bin/example-1
    # mpi_run $np ./bin/example-1
    logs
}

main
