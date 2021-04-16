#!/bin/sh
set -e

kungfu_run_flags() {
    local np=$1
    echo -q
    echo -logdir logs/bench
    echo -H "127.0.0.1:$np"
    echo -np $np
}

kungfu_run() {
    local np=$1
    shift
    kungfu-run $(kungfu_run_flags $np) $@
}

main() {
    # kungfu_run 4 kungfu-bench-allreduce -model resnet50-imagenet -mode seq -epochs 10 -warmup 0
    # kungfu_run 4 kungfu-bench-allreduce -model vgg16-imagenet -mode seq -epochs 10 -warmup 0
    kungfu_run 4 kungfu-bench-allreduce -model bert -mode seq -epochs 10 -warmup 0
}

main
