#!/bin/bash

OPTIND=1

variant=core
level=release

while getopts "hv:l:" opt; do
    case "$opt" in
    h)
        echo "usage: make.sh [-h] [-v variant] [-l debug-level]"
        echo "       -h show help"
        echo "       -v choose jvm-variant (server, client, minimal, core, zero, zeroshark, custom). default is core"
        echo "       -l choose debug level (release, fastdebug, slowdebug, optimized). default is release"
        exit 0
        ;;
    v)  variant=$OPTARG
        ;;
    l)  level=$OPTARG
        ;;
    esac
done

make CONF=linux-riscv64-$variant-$level