#!/bin/sh
set -e

. ./scripts/profile/measure.sh

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir logs/bench/$logdir

    echo -logfile kungfu-run.log
    # echo -v false

    echo -H "127.0.0.1:$np"
    echo -np $np

    echo -strategy STAR
    # echo -strategy RING
}

kungfu_run() {
    local np=$1
    shift
    local logdir=$1
    shift

    kungfu-run $(kungfu_run_flags $np $logdir) $@
}

bench_workload() {
    local workload=$1
    local times=$2
    kungfu_run 4 $workload ./bin/bench-all-reduce $workload $times
}

measure bench_workload 1024x100 20

export STDML_COLLECTIVE_USE_THREAD_POOL=1
export STDML_COLLECTIVE_USE_ASYNC=1
measure bench_workload 1024x100 20
