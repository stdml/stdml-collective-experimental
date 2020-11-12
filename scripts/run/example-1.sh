#!/bin/sh
set -e

kungfu_run_flags() {
    # echo -q
    echo -logdir logs
    echo -H "127.0.0.1:4 "
    echo -np 4
}

kungfu_run() {
    env KUNGFU_CONFIG_LOG_LEVEL=debug \
        kungfu-run $(kungfu_run_flags) $@
}

main() {
    kungfu_run ./bin/example-1
}

main
