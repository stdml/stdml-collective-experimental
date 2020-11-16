#!/bin/sh
set -e

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir logs/profile/$logdir
    echo -logfile kungfu-run.log

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
    kungfu_run 4 1024x100 ./bin/bench-all-reduce 1024x10 1
    # kungfu_run 4 1024x100 kungfu-bench-allreduce
}

parse_timeline() {
    local filename=$1
    grep '``' $filename | awk '{if ($6 == "4096") print $3, $4, $2}'
}

style() {
    echo "send red"
    echo "read_body green"
}

summary_all() {
    parse_timeline logs/profile/1024x100/127.0.0.1.10000.stdout.log >0.events
    parse_timeline logs/profile/1024x100/127.0.0.1.10001.stdout.log >1.events
    parse_timeline logs/profile/1024x100/127.0.0.1.10002.stdout.log >2.events
    parse_timeline logs/profile/1024x100/127.0.0.1.10003.stdout.log >3.events

    style >style.txt
    vis-interval 0.events,1.events,2.events,3.events x.bmp 12,4,4,4 3 3
    # vis-interval 0.events,1.events,2.events,3.events x.png 12,4,4,4 3 3
}

main() {
    profile_all
    summary_all
}

main
