# Running qemu emulator standalone

Install `qemu-system-risc64` and follow instructions at https://wiki.qemu.org/Documentation/Platforms/RISCV#Booting_Linux.

# Running qemu emulator in Docker

Install `docker`. To start the emulator, run 
 
    $ docker run -p 2222:22 --rm azulresearch/dev-riscv

or (from this directory)

    $ docker build --tag=riscv-emulator .
    $ docker run -p 2222:22 --rm riscv-emulator

You can then access the emulator using ssh:

    $ ssh root@127.0.0.1 -p 2222

Login is `root`, password is `riscv`.
