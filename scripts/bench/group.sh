#!/bin/sh
set -e

cfg_flags() {
    echo --benchmarks
}

build() {
    ./configure $(cfg_flags)
    make -j $(nproc)
}

. ./scripts/profile/measure.sh
. ./scripts/bin/stdml-collective-run

bench_workload() {
    local workload=$1
    local steps=$2
    local warms=$3

    export STDML_COLLECTIVE_LOG_DIR=logs/bench-group/$workload
    stdml_collective_run_n 4 ./bin/bench-group-all-reduce $workload $steps $warms
}

measure build

workload=1024x100
measure bench_workload $workload 20 5
