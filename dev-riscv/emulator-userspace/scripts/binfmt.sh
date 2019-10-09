#!/bin/sh

binfmt_id="qemu-riscv64"
binfmt_path="/proc/sys/fs/binfmt_misc"
qemu_path="/usr/bin/qemu-riscv64-static"

if [ "x$1" = "xregister" ]; then
    echo ':'$binfmt_id':M::\x7f\x45\x4c\x46\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xf3\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:'$qemu_path':CF' > "$binfmt_path/register"
elif [ x"$1" = "xunregister" ]; then
    echo -1 > "$binfmt_path/$binfmt_id"
else
    echo "Usage: $0 <register|unregister>"
    exit 1
fi
