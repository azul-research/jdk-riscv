#!/bin/bash

set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <test input> <resulting exe>"
    exit 1
fi

input=$1
output=$2.c
exe=$2
prefix=riscv64-unknown-linux-gnu-
cc="${prefix}gcc"

cat > "$output" << EOF

#include <stdio.h>

int main(void) {
    unsigned long res = 0xbadc0de;
    asm(
EOF
cat "$input" >> "$output"
cat >> "$output" << EOF
    : "=r"(res)
    );
    printf("%lu\\n", res);
    return 0;
}
EOF

$cc -static "$output" -o "$exe"
rm -f "$output"
