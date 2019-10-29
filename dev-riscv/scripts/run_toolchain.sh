#!/bin/bash

docker run --rm -it -v "$(pwd)":/jdk-riscv --network host tsarn/riscv-toolchain