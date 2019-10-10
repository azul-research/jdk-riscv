# Building the RISC-V port

## Required dependencies on host

Install `build-essential` (or equivalent) and Java 12 or 13.

Install the toolchain (https://github.com/riscv/riscv-gnu-toolchain).

## Required dependencies on target

OpenJDK apparently requires all of them.
Substute `/opt/riscv` for your toolchain prefix.

Install all of the following dependencies in order:

### libffi

    $ git clone https://github.com/libffi/libffi
    $ cd libffi
    $ ./autogen.sh
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### cups

    $ git clone https://github.com/apple/cups
    $ cd cups
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --disable-ssl --disable-gssapi --disable-avahi --disable-libusb --disable-dbus --disable-systemd
    $ make
    # make install

### expat

Required by `fontconfig` and `xorg`

    $ git clone https://github.com/libexpat/libexpat
    $ cd libexpat/expat
    $ ./buildconf.sh
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### zlib

Required by `freetype` and `xorg`

    $ git clone https://github.com/madler/zlib
    $ cd zlib
    $ CHOST=riscv64 CC=riscv64-unknown-linux-gnu-gcc AR=riscv64-unknown-linux-gnu-ar RANLIB=riscv64-unknown-linux-gnu-ranlib ./configure  --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### libpng

Required by `zlib`, `freetype` and `xorg`

    $ git clone https://github.com/glennrp/libpng
    $ cd libpng
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### freetype

Required by `fontconfig`

    $ git clone https://git.savannah.nongnu.org/git/freetype/freetype2.git
    $ cd freetype2
    $ ./autogen.sh
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --with-brotli=no --with-harfbuzz=no --with-bzip2=no
    $ make
    # make install

### json-c

Required by `fontconfig`

    $ git clone https://github.com/json-c/json-c
    $ cd json-c
    $ ./autogen.sh
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### fontconfig

    $ git clone https://gitlab.freedesktop.org/fontconfig/fontconfig
    $ cd fontconfig
    $ ./autogen.sh --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr

### alsa

    $ git clone https://github.com/alsa-project/alsa-lib
    $ cd alsa-lib
    $ libtoolize --force --copy --automake && aclocal && autoheader && automake --foreign --copy --add-missing && autoconf
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr
    $ make
    # make install

### libuuid

Required by `xorg`

    $ git clone https://github.com/karelzak/util-linux
    $ cd util-linux
    $ ./configure --host=riscv64-unknown-linux-gnu --prefix=/opt/riscv/sysroot/usr --disable-all-programs --enable-libuuid
    $ make
    # make install

### Xorg

    $ mkdir xorg
    $ cd xorg
    $ cp <THIS REPO>/dev-riscv/xorg_modules .
    $ git clone git://anongit.freedesktop.org/git/xorg/util/modular util/modular
    # CONFFLAGS="--host=riscv64-unknown-linux-gnu --disable-malloc0returnsnull" ./util/modular/build.sh --modfile xorg_modules --clone /opt/riscv/sysroot/usr

## Building OpenJDK zero

```
bash configure \
    --with-jvm-variants=zero \
    --disable-warnings-as-errors \
    --openjdk-target=riscv64-unknown-linux-gnu \
    --with-freetype=bundled \
    --with-sysroot=/opt/riscv/sysroot \
    --x-includes=/opt/riscv/sysroot/usr/include \
    --x-libraries=/opt/riscv/sysroot/usr/lib \
    && make
```
