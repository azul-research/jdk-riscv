#!/bin/sh

set -e

rootfs="/riscv"

chroot $rootfs apt-get update
chroot $rootfs apt-get install -y build-essential git vim gdb
