#!/bin/sh
set -e

./scripts/group-by-thread.rb logs/bench-group/1024x100/* >all.log
./scripts/euler.rb all.log >reduce.log
