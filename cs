#!/bin/sh

set -e

./configure && make -j8

./bin/example-rchan s &
pid=$!

./bin/example-rchan c

sleep 3
kill -s INT $pid
