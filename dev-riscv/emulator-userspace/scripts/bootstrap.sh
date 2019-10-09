#!/bin/sh

set -e

rootfs="/riscv"

apt-get update
apt-get install -y qemu-user-static debootstrap debian-ports-archive-keyring
mkdir -p $rootfs/usr/bin
cp /usr/bin/qemu-riscv64-static $rootfs/usr/bin
debootstrap --arch=riscv64 --keyring /usr/share/keyrings/debian-ports-archive-keyring.gpg --include=debian-ports-archive-keyring unstable $rootfs/ http://deb.debian.org/debian-ports
