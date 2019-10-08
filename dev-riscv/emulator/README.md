# Running qemu emulator standalone

Install `qemu-system-risc64` and follow instructions at https://wiki.qemu.org/Documentation/Platforms/RISCV#Booting_Linux.

# Running qemu emulator in Docker

Install `docker` and from this directory run:

    $ docker build --tag=riscv-emulator .

To start the emulator, run

    $ docker run -p 2222:22 --rm riscv-emulator

You can then access the emulator using ssh:

    $ ssh root@127.0.0.1 -p 2222

Login is `root`, password is `riscv`.
