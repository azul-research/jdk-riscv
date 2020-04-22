#!/bin/bash

OPTIND=1

variants=core
level=slowdebug

while getopts "hv:l:" opt; do
    case "$opt" in
    h)
        echo "usage: $0 [-h] [-v jvm-variants] [-l debug-level]"
        echo "       -h show help"
        echo "       -v set --with-jvm-variants flag (server, client, minimal, core, zero, zeroshark, custom). default is core"
        echo "       -l set --with-debug-level flag (release, fastdebug, slowdebug, optimized). default is slowdebug"
        exit 0
        ;;
    v)  variants=$OPTARG
        ;;
    l)  level=$OPTARG
        ;;
    esac
done

cd /jdk-riscv

bash configure \
    --with-jvm-variants=$variants \
    --with-debug-level=$level \
    --with-native-debug-symbols=internal \
    --disable-warnings-as-errors \
    --openjdk-target=riscv64-unknown-linux-gnu \
    --with-sysroot=/opt/riscv/sysroot \
    --x-includes=/opt/riscv/sysroot/usr/include \
    --x-libraries=/opt/riscv/sysroot/usr/lib
