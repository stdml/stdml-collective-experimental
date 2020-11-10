#!/bin/sh
set -e

kungfu_run_flags() {
    echo -q
    echo -logdir logs
    echo -np 4
}

kungfu_run() {
    kungfu-run $(kungfu_run_flags) $@
}

main() {
    kungfu_run ./bin/example-1
}

main
