#!/bin/sh
set -e

cd $(dirname $0)/../..
. ./scripts/bin/stdml-collective-run

stdml_collective_run_n 4 ./bin/test-shutdown
