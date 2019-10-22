# Cross-compiling zero build quickstart

First of all you need to download riscv compiler toolchain ([issue #1](https://github.com/azul-research/jdk-riscv/issues/1) / [full instruction](https://github.com/azul-research/jdk-riscv/tree/riscv/dev-riscv/toolchain)):

Go to repository jdk-riscv folder and switch to branch zero-compiling

Don't forget to mount this repository to docker by flag -v

    $ docker run --rm -v $PWD:/home/rep -it tsarn/riscv-toolchain

In the docker image in you should run this commands to compile open-jdk

    $ cd /home/rep
    $ bash configure \
    --with-jvm-variants=zero \
    --disable-warnings-as-errors \
    --openjdk-target=riscv64-unknown-linux-gnu \
    --with-sysroot=/opt/riscv/sysroot \
    --x-includes=/opt/riscv/sysroot/usr/include \
    --x-libraries=/opt/riscv/sysroot/usr/lib \
    && make

Now open another terminal and copy sysroot to repository folder ([issue #2](https://github.com/azul-research/jdk-riscv/issues/2))

    $ docker ps # you should find image id and run second command with your id
    >> f2d86469ddb6        tsarn/riscv-toolchain
    $ docker cp f2d86469ddb6:/opt/riscv/sysroot riscv-sysroot

Now open docker qemu emulator of riscv and check that open-jdk works ([issue #7](https://github.com/azul-research/jdk-riscv/issues/7))

    $ docker run -p 2222:22 --rm -it -v $PWD:/home/rep azulresearch/riscv-emu
    $ cd /home/rep
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/riscv-sysroot/usr/lib/
    $ time ./build/linux-riscv64-zero-release/jdk/bin/java -version

Last command will take approximately 1-2 minutes
