# Quickstart

To start the container, run:

    $ docker run --rm -it tsarn/riscv-toolchain

Alternatively, you can build the Docker image yourself:

    $ docker build --tag=riscv-toolchain .

This way, to start the container, run:

    $ docker run --rm -it riscv-toolchain

# Usage

Everything is in `/opt/riscv`.
System root is in `/opt/riscv/sysroot`.
To build OpenJDK, install java 12 or 13 and run:

```
bash configure \
    --with-jvm-variants=zero \
    --disable-warnings-as-errors \
    --openjdk-target=riscv64-unknown-linux-gnu \
    --with-sysroot=/opt/riscv/sysroot \
    --x-includes=/opt/riscv/sysroot/usr/include \
    --x-libraries=/opt/riscv/sysroot/usr/lib \
    && make
```
