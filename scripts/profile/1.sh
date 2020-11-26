#!/bin/sh
set -e

. ./scripts/profile/measure.sh

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

kungfu_run_flags() {
    local np=$1
    local logdir=$2

    echo -q
    echo -logdir logs/profile/$logdir
    echo -logfile kungfu-run.log
    echo -v=false

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
        STDML_COLLECTIVE_ENABLE_LOG=1 \
        STDML_COLLECTIVE_ENABLE_TRACE=1 \
        STDML_COLLECTIVE_USE_ASYNC=0 \
        kungfu-run $(kungfu_run_flags $np $logdir) $@
}

profile_one() {
    local size=$1
    local count=$2
    local name="${size}x${count}"
    kungfu_run 4 $name ./bin/bench-all-reduce $name 4
}

parse_timeline() {
    # local suffix=$1
    local size=$2
    local filename=$3
    local key_size=$((size * 4))
    grep '``' $filename | awk "{if (\$6 == \"$key_size\") print \$3, \$4, \$2 \"$suffix\"}"
}

style() {
    echo "send #ffff00"
    echo "read_body #00ff00"
    echo "add_to #ff0000"

    # echo "send0 #ffff00"
    # echo "read_body0 #00ff00"

    # echo "send1 #ffff00"
    # echo "send2 #ffff00"
    # echo "send3 #ffff00"

    # echo "read_body1 #00ff00"
    # echo "read_body2 #00ff00"
    # echo "read_body3 #00ff00"
}

summary_one() {
    local size=$1
    local count=$2
    local name="${size}x${count}"
    parse_timeline 0 $size logs/profile/$name/127.0.0.1.10000.stdout.log | tail -n +1 >0.events
    parse_timeline 1 $size logs/profile/$name/127.0.0.1.10001.stdout.log | tail -n +1 >1.events
    parse_timeline 2 $size logs/profile/$name/127.0.0.1.10002.stdout.log | tail -n +1 >2.events
    parse_timeline 3 $size logs/profile/$name/127.0.0.1.10003.stdout.log | tail -n +1 >3.events

    cat 0.events 1.events 2.events 3.events >all.events
    style >style.txt
    vis-interval '0.events,1.events,2.events,3.events;all.events' $name.png '18,4,4,4;30' 1 2 15
}

run_one() {
    local size=$1
    local count=$2
    local name="${size}x${count}"

    # if [ ! -d logs/profile/$name ]; then
    #     profile_one $size $count
    # fi
    profile_one $size $count
    summary_one $size $count
}

main() {
    run_one 1024 100
    # run_one 2048 100
    # run_one 4096 50
    # run_one 8192 20
    # run_one 10240 20
}

export STDML_COLLECTIVE_USE_THREAD_POOL=0
main
cp 1024x100.png 0.png

export STDML_COLLECTIVE_USE_THREAD_POOL=1
main
cp 1024x100.png 1.png

# export STDML_COLLECTIVE_USE_THREAD_POOL=0
# measure profile_one 1024 100
# export STDML_COLLECTIVE_USE_THREAD_POOL=1
# measure profile_one 1024 100

# 0 1 2 3 4 5 6 7 8 9 a b c d e f
#   x   x   x   x   x   x   x   x
#       x       x       x       x
