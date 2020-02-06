# Debugging riscv code with gdb

## Running gdb server on riscv qemu

Run docker with qemu with `--network host` flag or just run the following script:

`dev-riscv/scripts/run_qemu.sh`

Start gdb server: `QEMU_GDB=port path/to/executable`. For example, to debug java:

`QEMU_GDB=12345 /jdk-riscv/build/linux-riscv64-core-fastdebug/jdk/bin/java`

## Running gdb client in toolchain docker

Run docker with toolchain with `--network host` flag or just run the following script:

`dev-riscv/scripts/run_toolchain.sh`

Launch gdb client with the path to the same file as on server:

`riscv64-unknown-linux-gnu-gdb /jdk-riscv/build/linux-riscv64-core-fastdebug/jdk/bin/java`

Connect to gdb server (note that `:12345` is the same port as server uses):

`target remote :12345`

## Scripts for main gdb scenario

To start gdb server in qemu run the following script:

`dev-riscv/scripts/gdb_server.sh`

To allow use `.gdbinit` file in toolchain docker run the following script:

`dev-riscv/scripts/patch_gdbinit.sh` 

To start gdb client in toolchain docker run the following script:

`dev-riscv/scripts/gdb_client.sh`