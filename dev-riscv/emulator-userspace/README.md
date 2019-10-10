# Quickstart

Install using package manager, build from source or 
copy prebuilt QEMU to `/usr/bin/qemu-riscv64-static`:

    # cp ./bin/x86_64-linux-gnu/qemu-riscv64-static /usr/bin/

Configure the kernel to use the emulator to run RISC-V executables:

    # ./scripts/binfmt.sh register

Please note that the binfmt configuration doesn't persist across reboots.
If your distribution supports it, you can use `systemd-binfmt` to achieve that.
If you don't properly configure binfmt, then you won't be able to build or run
the Docker image.

To start the container, run:

    $ docker run --rm -it azulresearch/riscv-emu-user

Alternatively, you can build the Docker image yourself:

    $ docker build --tag=riscv-emu-user .

This way, to start the container, run:

    $ docker run --rm -it riscv-emu-user

# Building QEMU manually

Download latest version of QEMU at https://www.qemu.org/download/#source
You will also need static versions of glib and pcre.

Configure QEMU:

```
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
```

Build and install QEMU:

    $ make
    # cp riscv64-linux-user/qemu-riscv64 /usr/bin/qemu-riscv64-static
