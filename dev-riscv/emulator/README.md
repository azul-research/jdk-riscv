# Running qemu emulator standalone

Install `qemu-system-risc64` and follow instructions at https://wiki.qemu.org/Documentation/Platforms/RISCV#Booting_Linux.

# Running qemu emulator in Docker

Install `docker`. To start the emulator, run 
 
    $ docker run -it -p 2222:22 --rm azulresearch/riscv-emu

or (from this directory)

    $ docker build --tag=riscv-emulator .
    $ docker run -it -p 2222:22 --rm riscv-emulator

You can then access the emulator using ssh:

    $ ssh root@127.0.0.1 -p 2222

Login is `root`, password is `riscv`.

To exit from QEMU press <kbd>CTRL + A</kbd>, release and press <kbd>X</kbd>.
