file /jdk-riscv/build/linux-riscv64-core-slowdebug/jdk/bin/java

set solib-search-path /jdk-riscv/build/linux-riscv64-core-slowdebug/jdk/lib/:/jdk-riscv/build/linux-riscv64-core-slowdebug/support/modules_libs/java.base/server/:/jdk-riscv/build/linux-riscv64-core-slowdebug/jdk/lib/server/

target remote :12345

c
set sysroot /
t 2
set sysroot .
b javaCalls.cpp:439
c
