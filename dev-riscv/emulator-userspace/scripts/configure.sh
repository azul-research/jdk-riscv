#!/bin/sh

set -e

rootfs="/riscv"

run() {
    chroot "$rootfs" /usr/bin/qemu-riscv64-static -e /usr/bin/qemu-riscv64-static "$@"
}

run /usr/bin/apt-get update
run /usr/bin/apt-get install -y build-essential git gdb
