#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir logs/profile/$logdir

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

    env KUNGFU_CONFIG_LOG_LEVEL=debug \
        STDML_ENABLE_LOG=1 \
        STDML_ENABLE_TRACE=1 \
        kungfu-run $(kungfu_run_flags $np $logdir) $@
}

profile_all() {
    # kungfu_run 4 1024x100 ./bin/bench-all-reduce 1024x1000 1
    kungfu_run 4 1024x100 ./bin/bench-all-reduce 1024x100 1
    # kungfu_run 4 1024x100 kungfu-bench-allreduce
}

main() {
    profile_all
    # summary_all
}

main
