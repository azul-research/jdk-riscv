FROM debian:unstable AS builder

COPY ./scripts/bootstrap.sh /bootstrap.sh
RUN /bootstrap.sh

COPY ./scripts/configure.sh /configure.sh
RUN /configure.sh

FROM scratch

COPY --from=builder /riscv /

CMD "/bin/bash"
