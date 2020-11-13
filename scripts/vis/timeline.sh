#!/bin/bash
set -e

export LD_LIBRARY_PATH=$HOME/local/opencv/lib

for f in $(ls logs/bench/*.stdout.log); do
    echo $f
    cat $f | grep '``' | awk '{print $3, $4, $1}' >$f.events
done

join() {
    local IFS=','
    echo "$*"
}

filenames=$(join $(ls logs/bench/*.events))

echo $filenames
vis-interval $filenames x.png
