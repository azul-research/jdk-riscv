#!/bin/sh

set -e

rootfs="/riscv"

debootstrap --arch=riscv64 --foreign --keyring /usr/share/keyrings/debian-ports-archive-keyring.gpg --include=debian-ports-archive-keyring unstable $rootfs/ http://deb.debian.org/debian-ports

mkdir -p "$rootfs/usr/bin"
cp /usr/bin/qemu-riscv64-static "$rootfs/usr/bin/"

chroot "$rootfs" /usr/bin/qemu-riscv64-static --execve /usr/bin/qemu-riscv64-static /bin/bash /debootstrap/debootstrap --second-stage
