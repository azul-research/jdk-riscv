#!/bin/bash

OPTIND=1

variant=core
level=release
testfile=T1

while getopts "hv:l:t:" opt; do
    case "$opt" in
    h)
        echo "usage: $0 [-h] [-v variant] [-l debug-level]"
        echo "       -h show help"
        echo "       -v choose jvm-variant (server, client, minimal, core, zero, zeroshark, custom). default is core"
        echo "       -l choose debug level (release, fastdebug, slowdebug, optimized). default is release"
        exit 0
        ;;
    v)  variant=$OPTARG
        ;;
    l)  level=$OPTARG
        ;;
    t) testfile=$OPTARG
        ;;
    esac
done

java -jar dev-riscv/tests/asmtools.jar jasm dev-riscv/tests/${testfile}.jasm -d build/linux-riscv64-$variant-$level/jdk/modules/java.base
