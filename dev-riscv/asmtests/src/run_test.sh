#!/bin/bash

set -e

asm="./build/asm"
answer="./build/answer"
output="./build/output"
testexe="./build/testexe"

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <test name>"
    exit 1
fi

./build/tester "$1" "$asm" > "$answer"
./src/generate_stub.sh "$asm" "$testexe"
"$testexe" > "$output"

if ! cmp "$output" "$answer" -s ; then
    echo " FAIL $1"
    echo "See $output and $answer"
    exit 1
fi
echo "  OK   $1"
