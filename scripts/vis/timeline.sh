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
    ./scripts/vis/style.rb

    echo 'send_0 #ffff00'
    echo 'send_1 #ff0000'
    echo 'send_2 #00ff00'
    echo 'send_3 #0000ff'

    echo 'read_body_0 #3f3f00'
    echo 'read_body_1 #3f0000'
    echo 'read_body_2 #003f00'
    echo 'read_body_3 #00003f'

    echo 'run_graphs0_0 #f50cc2'
    echo 'recv_onto_0 #ff0000'
    echo 'send_onto_0 #f50cc2'

    echo 'run_graphs1_0 #007bce'

    echo 'read_body_0 #ff0000'
    echo 'q->get()_0 #3f3fff'
    # echo 'add_to_0 #ff0000'
    echo 'in_add_to_0 #5f0010'

    # echo 'recv_onto_0 red'
    # echo 'recv_onto_0 red'
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
    # vis resnet50 logs/bench/resnet50 $((214 * 2)) $((214 * 12))
    vis 1024x100 logs/bench/1024x100 $((10 * 2)) $((10 * 12))
    vis 102400x100 logs/bench/102400x100 $((10 * 2)) $((10 * 12))
    vis 1024000x100 logs/bench/1024000x100 $((10 * 2)) $((10 * 12))
    vis 10240000x10 logs/bench/10240000x10 $((10 * 2)) $((10 * 12))
    # vis resnet50-fuse logs/bench/resnet50-fuse 2 12
}

main
