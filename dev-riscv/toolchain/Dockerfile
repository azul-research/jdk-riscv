FROM debian:latest

RUN mkdir -p /build && apt-get update && apt-get install -y git autoconf automake autotools-dev \
    curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf \
    libtool patchutils bc zlib1g-dev libexpat-dev python2 python3


RUN cd /build && git clone --recursive https://github.com/riscv/riscv-gnu-toolchain && \
    cd riscv-gnu-toolchain && ./configure --prefix=/opt/riscv && make linux

ENV PATH="/opt/riscv/bin:${PATH}"

RUN cd /build && git clone --depth=1 https://github.com/libffi/libffi && cd libffi && ./autogen.sh && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://github.com/apple/cups && cd cups && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --disable-ssl \
    --disable-gssapi --disable-avahi --disable-libusb --disable-dbus --disable-systemd && \
    make && make install

RUN cd /build && git clone --depth=1 https://github.com/libexpat/libexpat && cd libexpat/expat && ./buildconf.sh \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://github.com/madler/zlib && cd zlib && \
    CHOST=riscv64 CC=riscv64-unknown-linux-gnu-gcc AR=riscv64-unknown-linux-gnu-ar RANLIB=riscv64-unknown-linux-gnu-ranlib \
    ./configure  --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://github.com/glennrp/libpng && cd libpng && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://git.savannah.nongnu.org/git/freetype/freetype2.git && cd freetype2 && \
    ./autogen.sh && ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --with-brotli=no --with-harfbuzz=no --with-bzip2=no && \
    make && make install

RUN cd /build && git clone --depth=1 https://github.com/json-c/json-c && cd json-c && ./autogen.sh && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://gitlab.freedesktop.org/fontconfig/fontconfig && \
    cd fontconfig && ./autogen.sh --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://github.com/alsa-project/alsa-lib && cd alsa-lib && \
    libtoolize --force --copy --automake && aclocal && autoheader && automake --foreign --copy --add-missing && autoconf && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr && make && make install

RUN cd /build && git clone --depth=1 https://github.com/karelzak/util-linux && cd util-linux && \
    ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --disable-all-programs --enable-libuuid && \
    make && make install

COPY xorg_modules /build

RUN cd /build && mkdir xorg && cd xorg && git clone --depth=1 git://anongit.freedesktop.org/git/xorg/util/modular util/modular && \
    CONFFLAGS="--host=riscv64-unknown-linux-gnu --disable-malloc0returnsnull" ./util/modular/build.sh --modfile /build/xorg_modules --clone /opt/riscv/sysroot/usr
