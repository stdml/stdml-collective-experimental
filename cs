#!/bin/sh

set -e

./configure && make -j8

./bin/example-rchan s &
pid=$!

./bin/example-rchan c

kill -s TERM $pid

echo "killed"
