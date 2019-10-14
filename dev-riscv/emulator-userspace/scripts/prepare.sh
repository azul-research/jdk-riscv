#!/bin/sh

set -e

rootfs="/riscv"

apt-get update
apt-get install -y build-essential git debootstrap debian-ports-archive-keyring python git glib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev

mkdir -p "$rootfs"

git clone https://github.com/azul-research/qemu --depth=1
cd qemu
./configure \
    --static \
    --target-list=riscv64-linux-user \
    --prefix=/usr \
    --sysconfdir=/etc \
    --localstatedir=/var \
    --libexecdir=/usr/lib/qemu \
    --enable-linux-user \
    --disable-debug-info \
    --disable-bsd-user \
    --disable-werror \
    --disable-system \
    --disable-tools \
    --disable-docs \
    --disable-gtk \
    --disable-gnutls \
    --disable-nettle \
    --disable-gcrypt \
    --disable-glusterfs \
    --disable-libnfs \
    --disable-libiscsi \
    --disable-vnc \
    --disable-libssh \
    --disable-libxml2 \
    --disable-vde \
    --disable-sdl \
    --disable-opengl \
    --disable-xen \
    --disable-fdt \
    --disable-vhost-net \
    --disable-vhost-crypto \
    --disable-vhost-user \
    --disable-vhost-vsock \
    --disable-vhost-scsi \
    --disable-tpm \
    --disable-qom-cast-debug \
    --disable-capstone
make
cp riscv64-linux-user/qemu-riscv64 /usr/bin/qemu-riscv64-static
