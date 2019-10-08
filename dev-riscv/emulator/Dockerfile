FROM debian:unstable

WORKDIR /qemu

RUN apt-get update && apt-get install -y wget qemu-system-misc xz-utils
RUN echo "Downloading bbl" && wget -q https://fedorapeople.org/groups/risc-v/disk-images/bbl && \
    echo "Downloading stage4" && wget -q https://fedorapeople.org/groups/risc-v/disk-images/stage4-disk.img.xz && \
    echo "Downloading vmlinux" && wget -q https://fedorapeople.org/groups/risc-v/disk-images/vmlinux && \
    echo "Decompressing stage4" && unxz stage4-disk.img.xz

CMD qemu-system-riscv64 \
   -nographic \
   -machine virt \
   -smp 4 \
   -m 2G \
   -kernel bbl \
   -object rng-random,filename=/dev/urandom,id=rng0 \
   -device virtio-rng-device,rng=rng0 \
   -append "console=ttyS0 ro root=/dev/vda" \
   -device virtio-blk-device,drive=hd0 \
   -drive file=stage4-disk.img,format=raw,id=hd0 \
   -device virtio-net-device,netdev=usernet \
   -netdev user,id=usernet,hostfwd=tcp::22-:22
