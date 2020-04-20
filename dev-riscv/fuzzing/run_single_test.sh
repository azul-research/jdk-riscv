#!/bin/bash

variant=core
level=slowdebug
jdk_riscv_path=$(pwd)/../..
sysroot_path=$jdk_riscv_path/riscv-sysroot

OPTIND=1

while getopts "hv:l:" opt; do
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
    esac
done

while true; do 
    python3 fuzz.py
    javac javafuzz/T1.java 2>/dev/null && break
done
java javafuzz.T1 > answer

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
    -XX:+CallTestMethod \
    -XX:TestMethodClass=javafuzz.T1 -XX:TestMethodName=test \
    -XX:+ExitAfterTestMethod" | sed -n 's/TestMethodValue [a-z]\+ //p' > output || exit 1

diff -w output answer || exit 1
printf "OK "
cat output
