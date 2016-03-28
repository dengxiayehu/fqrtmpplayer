#! /usr/bin/env bash
set -e

ABS_DIR="$(cd "$(dirname "$0")"; pwd)"

TARBALLS="$ABS_DIR/tarballs"
[ ! -d "$TARBALLS" ] && { \
    echo "No tarballs dir found"; return 1; }

echo "----------------------------"
echo "[*] Make NDK standalone toolchain"
echo "----------------------------"
TOOLCHAIN_PATH="$ABS_DIR/toolchain"
ANDROID_PLATFORM=android-9
TOOLCHAIN_PREFIX=arm-linux-androideabi-
GCC_VER=4.8
TOOLCHAIN_TOUCH="$TOOLCHAIN_PATH/.touch"
[ ! -f "$TOOLCHAIN_TOUCH" ] && { \
    "$ANDROID_NDK"/build/tools/make-standalone-toolchain.sh \
        --install-dir="$TOOLCHAIN_PATH" \
        --platform="$ANDROID_PLATFORM" \
        --toolchain="$TOOLCHAIN_PREFIX$GCC_VER" \
        --stl=stlport && \
    touch "$TOOLCHAIN_TOUCH"; }
export PATH="$TOOLCHAIN_PATH/bin:$PATH"
export CROSS_SYSROOT="$TOOLCHAIN_PATH/sysroot"
echo -e "Done\n"

CONTRIB_SRC="$ABS_DIR/contrib-${TOOLCHAIN_PREFIX%-}"
[ ! -d "$CONTRIB_SRC" ] && mkdir "$CONTRIB_SRC"

INSTALL="$ABS_DIR/install"

compile_openssl() {
    echo "----------------------------"
    echo "[*] Compile openssl"
    echo "----------------------------"

    local tarname="openssl-1.0.2g.tar.gz"
    local dstsrc="$CONTRIB_SRC/${tarname%.tar.gz}"
    local openssl_touch="$dstsrc/.touch"
    [ -f "$openssl_touch" ] && { echo -e "Done\n"; return 0; }

    cd "$ABS_DIR"
    [ ! -d "$dstsrc" ] && \
        tar xvf "$TARBALLS/$tarname" -C "$CONTRIB_SRC"
    cd "$dstsrc"
    ./Configure android-armv7 zlib-dynamic no-shared \
                --prefix="$INSTALL" \
                --cross-compile-prefix="$TOOLCHAIN_PREFIX"
    make depend
    make
    make install
    echo -e "Done\n"
    touch "$openssl_touch"
}

compile_rtmpdump() {
    echo "----------------------------"
    echo "[*] Compile rtmpdump"
    echo "----------------------------"

    local tarname="rtmpdump.tar.gz"
    local dstsrc="$CONTRIB_SRC/${tarname%.tar.gz}"
    local rtmpdump_touch="$dstsrc/.touch"
    [ -f "$rtmpdump_touch" ] && { echo -e "Done\n"; return 0; }

    cd "$ABS_DIR"
    [ ! -d "$dstsrc" ] && \
        tar xvf "$TARBALLS/$tarname" -C "$CONTRIB_SRC"
    cd "$dstsrc"
    sed -i 's/=-lpthread\>/=-pthread/g' Makefile
    make CROSS_COMPILE="$TOOLCHAIN_PREFIX" \
         INC=-I"$INSTALL/include" XLDFLAGS=-L"$INSTALL"/lib \
         prefix="$INSTALL" \
         install
    echo -e "Done\n"
    touch "$rtmpdump_touch"
}

compile_openssl
compile_rtmpdump
