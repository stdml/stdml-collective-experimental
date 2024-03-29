#!/bin/sh
set -e

. ./scripts/profile/measure.sh
. ./scripts/bin/stdml-collective-run

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

# benchmark=stdml-collective-bench-allreduce
benchmark=./bin/bench-all-reduce

profile_one() {
    local size=$1
    local count=$2
    local name="${size}x${count}"

    # export KUNGFU_CONFIG_LOG_LEVEL=debug
    # export STDML_COLLECTIVE_ENABLE_LOG=1
    export STDML_COLLECTIVE_ENABLE_TRACE=1
    # export STDML_COLLECTIVE_USE_ASYNC=0
    export STDML_COLLECTIVE_LOG_DIR=logs/profile/$name

    stdml_collective_run_n 4 $benchmark $name 10
}

parse_timeline() {
    # local suffix=$1
    local size=$2
    local filename=$3
    local key_size=$((size * 4))
    grep '``' $filename | awk "{if (\$7 == \"$key_size\") print \$4, \$5, \$2 \"$suffix\"}"
}

style() {
    echo "send #00ff00"
    echo "send_head #007f00"
    echo "send_body #00af00"
    echo "par_send_into #007f00"

    echo "read_header #7f7f00"
    echo "read_body #ffff00"
    echo "add_to #ff0000"
    echo "all_reduce #7070ff"

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
    vis-interval '0.events,1.events,2.events,3.events;all.events' $name.png '36,8,8,8;30' 1 2 15
}

run_one() {
    local size=$1
    local count=$2
    local name="${size}x${count}"

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

export STDML_COLLECTIVE_USE_THREAD_POOL=1
main
cp 1024x100.png 0.png

./scripts/profile/query-timeline.rb

# export STDML_COLLECTIVE_USE_THREAD_POOL=1
# main
# cp 1024x100.png 1.png

# export STDML_COLLECTIVE_USE_THREAD_POOL=0
# measure profile_one 1024 100
# export STDML_COLLECTIVE_USE_THREAD_POOL=1
# measure profile_one 1024 100

# 0 1 2 3 4 5 6 7 8 9 a b c d e f
#   x   x   x   x   x   x   x   x
#       x       x       x       x
