FROM debian:unstable AS builder

COPY ./qemu-user-execve.diff /

COPY ./scripts/prepare.sh /prepare.sh
RUN /prepare.sh

COPY ./scripts/bootstrap.sh /bootstrap.sh
RUN /bootstrap.sh

COPY ./scripts/configure.sh /configure.sh
RUN /configure.sh

FROM scratch

COPY --from=builder /riscv /

ENTRYPOINT ["/usr/bin/qemu-riscv64-static", "-execve", "/usr/bin/qemu-riscv64-static"]
CMD ["/bin/bash"]
