#!/bin/bash

variant=core
level=slowdebug
jdk_riscv_path=$(pwd)/../..
sysroot_path=$jdk_riscv_path/riscv-sysroot

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
    -XX:+CallTestMethod -XX:+TraceBytecodes -XX:+Verbose \
    -XX:TestMethodClass=javafuzz.T1 -XX:TestMethodName=test \
    -XX:+DisableClinit -XX:+ExitAfterTestMethod" || exit 1
