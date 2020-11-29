#!/bin/sh
set -e

. ./scripts/profile/measure.sh
. ./scripts/bin/stdml-collective-run

export STDML_COLLECTIVE_ENABLE_LOG=1

cfg_flags() {
    echo --benchmarks
}

build() {
    ./configure $(cfg_flags)
    make -j $(nproc)
}

bench_workload() {
    local workload=$1
    local steps=$2
    local warms=$3
    export STDML_COLLECTIVE_LOG_DIR=logs/bench/$workload
    stdml_collective_run_n 4 ./bin/bench-all-reduce $workload $steps $warms
}

measure build

workload=1024x100

# measure bench_workload $workload 20 5

# export STDML_COLLECTIVE_USE_THREAD_POOL=1
# measure bench_workload $workload 20 5

export STDML_COLLECTIVE_USE_ASYNC=1
measure bench_workload $workload 20 5
