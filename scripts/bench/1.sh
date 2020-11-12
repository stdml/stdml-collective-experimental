#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    echo -q
    echo -logdir logs/bench
    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    env KUNGFU_CONFIG_LOG_LEVEL=debug \
        kungfu-run $(kungfu_run_flags $np) $@
}

main() {
    kungfu_run 4 ./bin/bench-all-reduce testdata/small.txt 1
    kungfu_run 4 ./bin/bench-all-reduce testdata/resnet50.txt 10
}

main
