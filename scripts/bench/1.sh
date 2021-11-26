#!/bin/sh
set -e

. ./scripts/profile/measure.sh
. ./scripts/bin/stdml-collective-run

bench_one() {
    local name=$1
    local workload=$2
    export STDML_COLLECTIVE_LOG_DIR=logs/bench/$name
    stdml_collective_run_n 4 ./bin/stdml-collective-bench-allreduce $workload 10 3
}

run_test_set() {
    for workload in $(cat $1); do
        bench_one $workload $workload
    done
}

bench_all() {
    # run_test_set tests/data/test-set-1.txt
    measure bench_one resnet50 tests/data/resnet50.txt
    measure bench_one vgg16 tests/data/vgg16.txt
    # bench_one bert tests/data/bert.txt
}

summary() {
    for f in $(ls logs/bench/$1/*.stdout.log); do
        grep FINAL $f || true
    done
}

summary_all() {
    for d in $(ls logs/bench); do
        summary $d
    done
}

main() {
    bench_all
    summary_all
}

# export STDML_COLLECTIVE_USE_THREAD_POOL=0
# main

export STDML_COLLECTIVE_ENABLE_LOG=1
export STDML_COLLECTIVE_USE_THREAD_POOL=1
# export STDML_COLLECTIVE_USE_AFFINITY=1
main

# export STDML_COLLECTIVE_USE_ASYNC=1
# main
