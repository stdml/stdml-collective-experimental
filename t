#!/bin/sh
set -e

rebuild() {
    ./configure
    make -j 8
}

kungfu_run_flags() {
    local np=$1

    echo -q
    echo -logdir logs/test
    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    kungfu-run $(kungfu_run_flags $np) $@
}

trace() {
    echo "BEGIN $@"
    $@
    echo "END $@"
    echo
    echo
}

test_all() {
    for n in $(seq 1 16); do
        trace kungfu_run $n ./bin/test-all-reduce
    done
}

rebuild
test_all
