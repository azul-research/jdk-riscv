#!/bin/sh

set -e

rootfs="/riscv"

apt-get update
apt-get install -y build-essential wget debootstrap debian-ports-archive-keyring python git glib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev

mkdir -p /qemu-build
mkdir -p "$rootfs"

cd /qemu-build
wget "https://download.qemu.org/qemu-4.1.0.tar.xz"
tar -xf qemu-4.1.0.tar.xz
cd "qemu-4.1.0"
patch -p1 -i /qemu-user-execve.diff
./configure --static --target-list=riscv64-linux-user
make
cp riscv64-linux-user/qemu-riscv64 /usr/bin/qemu-riscv64-static
