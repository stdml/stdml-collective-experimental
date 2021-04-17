#!/bin/sh
set -e

# export STDML_COLLECTIVE_ENABLE_LOG=1

rebuild() {
    ./configure --tests
    make -j $(nproc)
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
    local max_np=$1
    local prog=$2
    for np in $(seq 1 $max_np); do
        trace kungfu_run $np "logs/test/np=$np" $prog
    done
}

main() {
    trace rebuild

    trace test_all 16 ./bin/test-broadcast

    export STDML_COLLECTIVE_USE_THREAD_POOL=0
    trace test_all 16 ./bin/test-all-reduce

    export STDML_COLLECTIVE_USE_THREAD_POOL=1
    trace test_all 16 ./bin/test-all-reduce

    trace test_all 16 ./bin/test-all-gather
}

main
