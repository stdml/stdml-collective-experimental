#!/bin/sh
set -e

./configure
make

./scripts/run/example-1.sh
# ./scripts/bench/1.sh
