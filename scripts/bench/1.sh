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

main() {
    # kungfu_run 4 ./bin/bench-all-reduce testdata/small.txt 10
    kungfu_run 4 resnet50 ./bin/bench-all-reduce testdata/resnet50.txt 2
    # kungfu_run 4 resnet50-fuse ./bin/bench-all-reduce testdata/resnet50.txt 10 fuse
    # kungfu_run 4 ./bin/bench-all-reduce testdata/vgg16.txt 10
    # kungfu_run 4 ./bin/bench-all-reduce testdata/bert.txt 10
}

main
