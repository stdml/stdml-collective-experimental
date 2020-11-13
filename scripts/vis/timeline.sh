#!/bin/bash
set -e

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

extract_events() {
    local suffix=0
    for f in $(ls logs/bench/*.stdout.log); do
        echo $f
        cat $f | grep '``' | awk "{printf \"%s %s %s_%d\n\", \$3, \$4, \$2, \"$suffix\"}" >$f.events
        suffix=$((suffix + 1))
        echo $suffix
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
}

extract_events
filenames=$(join $(ls logs/bench/*.events))

echo $filenames
style >style.txt
vis-interval $filenames x.png $((214 / 1))

cat logs/bench/*.events >all.events

vis-interval all.events y.png $((214 * 6))
