#!/bin/sh
set -e

add_bin_path_after() {
    export PATH=$PATH:$1/bin
}

add_bin_path_after $HOME/local/clang

cd $(dirname $0)/../..

rebuild() {
    ./configure --verbose
    make
}

check_cpp() {
    clang-tidy "$1"
}

fix_cpp() {
    clang-tidy -fix "$1"
}

list_cpp_srcs() {
    jq -r '.[].file' compile_commands.json
}

list_hdr_and_srcs() {
    find ./src -type f | grep .cpp$
    find ./src -type f | grep .hpp$
    find ./src -type f | grep .h$
}

for_all_cpp() {
    for src in $(list_cpp_srcs); do
        echo "$1 $src"
        $1 $src
    done
}

fmt_all_cpp() {
    for src in $(list_hdr_and_srcs); do
        echo "clang-format -i $src"
        clang-format -i $src
    done
}

fix_all_cpp() {
    for_all_cpp fix_cpp
}

check_all() {
    for_all check
}

main() {
    case $1 in
    --fmt-cpp)
        fmt_all_cpp
        ;;
    --fix-cpp)
        rebuild
        fix_all_cpp
        ;;
    *)
        echo "unknown argument $1"
        exit 1
        ;;
    esac
}

main $@
