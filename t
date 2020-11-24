#!/bin/sh
set -e

export STDML_ENABLE_LOG=0

rebuild() {
    ./configure --tests
    make -j 8
}

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir $logdir
    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    local logdir=$1
    shift
    kungfu-run $(kungfu_run_flags $np $logdir) $@
}

trace() {
    echo "BEGIN $@"
    $@
    echo "END $@"
    echo
    echo
}

test_all() {
    for np in $(seq 1 16); do
        trace kungfu_run $np "logs/test/np=$np" ./bin/test-all-reduce
    done
}

trace rebuild

export STDML_USE_THREAD_POOL=0
trace test_all

export STDML_USE_THREAD_POOL=1
trace test_all

export STDML_COLLECTIVE_USE_ASYNC=1
trace test_all
