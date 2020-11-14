#!/bin/sh
set -e

mpi_run() {
    local np=$1
    shift
    mpirun -np $np $@
}

mpi_run() {
    local np=$1
    shift
    mpirun -H 127.0.0.1:4 -np $np $@
}

main() {
    local np=4
    mpi_run $np ./bin/example-1
}

main
