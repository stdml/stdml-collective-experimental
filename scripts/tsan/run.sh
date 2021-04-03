#!/bin/sh
set -e

. ./scripts/bin/stdml-collective-run

cfg_flags() {
    # echo --mpi
    # echo --go-runtime
    # echo --with-go-runtime=$HOME/local
    # echo --enable-elastic
    echo --tests
}

rebuild() {
    ./configure $(cfg_flags)
    make -j 8
}

check_leak() {
    local bin=$1
    local result=result.xml
    if [ ! -z "$2" ]; then
        result=$2
    fi

    valgrind --xml=yes --xml-file=${result} -q --leak-check=full ${bin}

    $(dirname $0)/analysis-valgrind-result.rb ${result}
}

check_leak_distributed() {
    local bin=$1
    export STDML_COLLECTIVE_LOG_DIR=logs/check-leak
    stdml_collective_run_n 4 valgrind --xml=yes --xml-fd=1 --leak-check=full ${bin}

    for xml in $(ls $STDML_COLLECTIVE_LOG_DIR/*stdout.log); do
        $(dirname $0)/analysis-valgrind-result.rb ${xml}
    done
}

main() {
    rebuild
    test_prog=./bin/test-leak
    check_leak $test_prog
    check_leak_distributed $test_prog
}

# $(dirname $0)/analysis-valgrind-result.rb result.xml
main
