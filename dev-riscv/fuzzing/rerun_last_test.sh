#!/bin/bash

variant=core
level=slowdebug
jdk_riscv_path=$(pwd)/../..
sysroot_path=$jdk_riscv_path/riscv-sysroot
quiet=

OPTIND=1

while getopts "hqv:l:" opt; do
    case "$opt" in
    h)
        echo "usage: $0 [-h] [-v variant] [-l debug-level]"
        echo "       -h show help"
        echo "       -q do not trace bytecodes"
        echo "       -v choose jvm-variant (server, client, minimal, core, zero, zeroshark, custom). default is core"
        echo "       -l choose debug level (release, fastdebug, slowdebug, optimized). default is release"
        exit 0
        ;;
    v)  variant=$OPTARG
        ;;
    l)  level=$OPTARG
        ;;
    q)  quiet=1
        ;;
    esac
done

extraopts=
if [ "x$quiet" = "x" ]; then
    extraopts="-XX:+TraceBytecodes -XX:+Verbose"
fi

javac javafuzz/T1.java
java javafuzz.T1 > answer
printf "Answer is: "
cat answer

docker run --rm -it \
    -v "$jdk_riscv_path":/jdk-riscv \
    -v "$sysroot_path":/opt/riscv/sysroot \
    -e LD_LIBRARY_PATH=/opt/riscv/sysroot/usr/lib/ \
    -w /jdk-riscv \
    --network host \
    azulresearch/riscv-emu-user \
    /bin/bash -c "cp /jdk-riscv/dev-riscv/fuzzing/javafuzz/T1.class \
    /jdk-riscv/build/linux-riscv64-$variant-$level/jdk/modules/java.base/javafuzz/ && \
    /jdk-riscv/build/linux-riscv64-$variant-$level/jdk/bin/java \
    -XX:+CallTestMethod $extraopts \
    -XX:TestMethodClass=javafuzz.T1 -XX:TestMethodName=test \
    -XX:+ExitAfterTestMethod" || exit 1
