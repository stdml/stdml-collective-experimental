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
    kungfu_run 4 10240x10 ./bin/bench-all-reduce 10240x10 1
    # kungfu_run 4 1024x100 kungfu-bench-allreduce
}

parse_timeline() {
    local suffix=$1
    local filename=$2
    grep '``' $filename | awk "{if (\$6 == \"40960\") print \$3, \$4, \$2 \"$suffix\"}"
}

style() {
    echo "send0 red"
    echo "read_body0 green"

    echo "send1 red"
    echo "send2 red"
    echo "send3 red"

    echo "read_body1 green"
    echo "read_body2 green"
    echo "read_body3 green"
}

summary_all() {
    name=10240x10
    parse_timeline 0 logs/profile/$name/127.0.0.1.10000.stdout.log >0.events
    parse_timeline 1 logs/profile/$name/127.0.0.1.10001.stdout.log >1.events
    parse_timeline 2 logs/profile/$name/127.0.0.1.10002.stdout.log >2.events
    parse_timeline 3 logs/profile/$name/127.0.0.1.10003.stdout.log >3.events

    style >style.txt
    # vis-interval 0.events,1.events,2.events,3.events x.bmp 12,4,4,4 3 3
    vis-interval 0.events,1.events,2.events,3.events x.png 12,4,4,4 100
}

main() {
    # profile_all
    summary_all
}

main
