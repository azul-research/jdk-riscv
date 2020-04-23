#!/bin/bash

if [ "$#" -gt 2 ]; then
	echo "usage: $0 [path/to/sysroot [path/to/jdk-riscv]]"	
	exit 1
fi

sysroot_path="$(pwd)/riscv-sysroot"
jdk_riscv_path="$(pwd)"

if [ "$#" -gt 0 ]; then
	case $1 in
  		/*) sysroot_path="$1" ;;
  		*) 	sysroot_path="$(pwd)/$1" ;;
	esac
fi

if [ "$#" -gt 1 ]; then
	case $2 in
  		/*) jdk_riscv_path="$2" ;;
  		*) 	jdk_riscv_path="$(pwd)/$2" ;;
	esac
fi

docker run --rm -it \
    -v "$jdk_riscv_path":/jdk-riscv \
    -v "$sysroot_path":/opt/riscv/sysroot \
    -e LD_LIBRARY_PATH=/opt/riscv/sysroot/usr/lib/ \
    -w /jdk-riscv \
    --network host \
    azulresearch/riscv-emu-user
 