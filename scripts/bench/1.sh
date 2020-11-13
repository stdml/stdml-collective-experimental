#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir logs/bench/$logdir

    echo -logfile kungfu-run.log
    # echo -v false

    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    local logdir=$1
    shift

    env KUNGFU_CONFIG_LOG_LEVEL=debug \
        kungfu-run $(kungfu_run_flags $np $logdir) $@
}

bench_all() {
    # kungfu_run 4 ./bin/bench-all-reduce testdata/small.txt 10
    # kungfu_run 4 1024x100 ./bin/bench-all-reduce 1024x100 2
    # kungfu_run 4 102400x100 ./bin/bench-all-reduce 102400x100 2
    # kungfu_run 4 1024000x100 ./bin/bench-all-reduce 1024000x100 2
    kungfu_run 4 10240000x10 ./bin/bench-all-reduce 10240000x10 2

    # kungfu_run 4 resnet50 ./bin/bench-all-reduce testdata/resnet50.txt 2
    # kungfu_run 4 resnet50-fuse ./bin/bench-all-reduce testdata/resnet50.txt 10 fuse
    # kungfu_run 4 ./bin/bench-all-reduce testdata/vgg16.txt 10
    # kungfu_run 4 ./bin/bench-all-reduce testdata/bert.txt 10
}

summary() {
    echo $1
    for f in $(ls logs/bench/$1/*.stdout.log); do
        grep FINAL $f
    done
}

summary_all() {
    for d in $(ls logs/bench); do
        echo $d
        summary $d
    done
}

main() {
    bench_all
    summary_all
}

main
