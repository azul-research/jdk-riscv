#!/bin/bash

docker run --rm -it \
    -v "$(pwd)":/jdk-riscv \
    -e LD_LIBRARY_PATH=/jdk-riscv/riscv-sysroot/usr/lib/ \
    --network host \
    azulresearch/riscv-emu-user