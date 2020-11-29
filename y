#!/bin/sh
set -e

# ./scripts/group-by-thread.rb logs/bench-group/1024x100/* >all.log
# ./scripts/euler.rb all.log >reduce.log

# grep send logs/bench/1024x100/*
grep recv logs/bench/1024x100/* | grep recv_onto
# grep recv logs/bench/1024x100/* | grep recv_into
