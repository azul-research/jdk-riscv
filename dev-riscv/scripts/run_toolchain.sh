#!/bin/bash

if [ "$#" -gt 1 ]; then
	echo "usage: $0 [path/to/jdk-riscv]"
	exit 1
fi

jdk_riscv_path="$(pwd)"

if [ "$#" -eq 1 ]; then
	case $1 in
  		/*) jdk_riscv_path="$1" ;;
  		*) 	jdk_riscv_path="$(pwd)/$1" ;;
	esac
fi

docker run --rm -it -v "$jdk_riscv_path":/jdk-riscv -w /jdk-riscv --network host tsarn/riscv-toolchain

