#!/bin/bash
set -e

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

extract_events() {
    local logdir=$1
    local suffix=0
    for f in $(ls $logdir/*.stdout.log); do
        echo $f
        cat $f | grep '``' | awk "{printf \"%s %s %s_%d\n\", \$3, \$4, \$2, \"$suffix\"}" >$f.events
        suffix=$((suffix + 1))
    done
}

join() {
    local IFS=','
    echo "$*"
}

style() {
    echo 'send_0 red'
    echo 'send_1 green'
    echo 'send_2 blue'
    echo 'send_3 yellow'

    echo 'read_body_0 grey'
    echo 'read_body_1 grey'
    echo 'read_body_2 grey'
    echo 'read_body_3 grey'
}

vis() {
    local name=$1
    local logdir=$2
    local h1=$3
    local h2=$4

    extract_events $logdir
    local filenames=$(join $(ls $logdir/*.events))

    style >style.txt
    vis-interval $filenames $name.x.png $h1

    cat $logdir/*.events >all.events

    vis-interval all.events $name.y.png $h2
}

main() {
    vis resnet50 logs/bench/resnet50 $((214 * 2)) $((214 * 12))
    # vis resnet50-fuse logs/bench/resnet50-fuse 2 12
}

main
