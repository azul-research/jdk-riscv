#!/bin/bash

OPTIND=1

variant=core
level=slowdebug
debug=
testFlags=''

while getopts "hdv:l" opt; do
    case "$opt" in
    h)
        echo "usage: $0 [-h] [-v variant] [-l debug-level]"
        echo "       -h show help"
        echo "       -v choose jvm-variant (server, client, minimal, core, zero, zeroshark, custom). default is core"
        echo "       -l choose debug level (release, fastdebug, slowdebug, optimized). default is slowdebug"
        echo "       -d launch gdb server"
        exit 0
        ;;
    v)  variant=$OPTARG
        ;;
    l)  level=$OPTARG
        ;;
    d)  debug=1
        ;;
    esac
done

executable="/jdk-riscv/build/linux-riscv64-$variant-$level/jdk/bin/java"
args="-XX:+TraceBytecodes -XX:+Verbose -XX:+CallTestMethod -XX:TestMethodClass=javafuzz.T1 -XX:TestMethodName=test -XX:+UseHeavyMonitors -XX:-RewriteBytecodes -XX:+PrintAssembly"

if [ "x$debug" = "x" ]; then
    $executable $args
else
    QEMU_GDB=12345 $executable $args -XX:+BreakAtStartup
fi
