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
    local steps=$2
    local warms=$3
    kungfu_run 4 $workload ./bin/bench-all-reduce $workload $steps $warms
}

workload=1024x100

# measure bench_workload $workload 20 5

# export STDML_COLLECTIVE_USE_THREAD_POOL=1
# measure bench_workload $workload 20 5

export STDML_COLLECTIVE_USE_ASYNC=1
measure bench_workload $workload 20 5
