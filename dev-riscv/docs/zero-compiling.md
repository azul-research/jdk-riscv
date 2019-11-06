# Cross-compiling zero build quickstart

## Build jdk zero

Go to repository `jdk-riscv` folder and switch to branch `riscv`

### With scripts

You need to download and start docker with toolchain

    $ ./dev-riscv/scripts/run_toolchain.sh

Now in the docker image you need to configure and build jdk

    $ cd /jdk-riscv # mounted folder with repository
    $ ./dev-riscv/scripts/configure.sh -v zero
    $ ./dev-riscv/scripts/make.sh -v zero

### Manual

First of all you need to download riscv compiler toolchain ([issue #1](https://github.com/azul-research/jdk-riscv/issues/1) / [full instruction](https://github.com/azul-research/jdk-riscv/tree/riscv/dev-riscv/toolchain)):

Don't forget to mount this repository folder to docker image by flag `-v`

    $ docker run --rm -v "$PWD":/jdk-riscv -it tsarn/riscv-toolchain

In the docker image in you should run this commands to compile open-jdk

    $ cd /jdk-riscv
    $ bash configure \
    --with-jvm-variants=zero \
    --disable-warnings-as-errors \
    --openjdk-target=riscv64-unknown-linux-gnu \
    --with-sysroot=/opt/riscv/sysroot \
    --x-includes=/opt/riscv/sysroot/usr/include \
    --x-libraries=/opt/riscv/sysroot/usr/lib \
    && make
    
## Run qemu and check

### With scripts

In new terminal go to the repository folder and run script to open docker image with qemu

    $ ./dev-riscv/scripts/run_qemu.sh
    
Now you can check that it is works
    
    $ time ./build/linux-riscv64-zero-release/jdk/bin/java -version

### Manual

Now open another terminal and copy `sysroot` to repository folder ([issue #2](https://github.com/azul-research/jdk-riscv/issues/2))

    $ docker ps # you should find image id and run second command with your id
    >> f2d86469ddb6        tsarn/riscv-toolchain
    $ docker cp f2d86469ddb6:/opt/riscv/sysroot riscv-sysroot

Now open `qemu` emulator of riscv with docker image and check that `open-jdk` works ([issue #7](https://github.com/azul-research/jdk-riscv/issues/7))

    $ docker run --rm -it -v "$PWD":/jdk-riscv azulresearch/riscv-emu-user
    $ cd /jdk-riscv
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"$PWD"/riscv-sysroot/usr/lib/
    $ time ./build/linux-riscv64-zero-release/jdk/bin/java -version

Last command will take approximately 1-2 minutes
