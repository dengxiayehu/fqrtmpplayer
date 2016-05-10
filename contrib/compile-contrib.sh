#! /usr/bin/env bash
set -e

ABS_DIR="$(cd "$(dirname "$0")"; pwd)"

echo "----------------------------"
echo "[*] Make NDK standalone toolchain"
echo "----------------------------"
TOOLCHAIN_PATH="$ABS_DIR/toolchain"
ANDROID_PLATFORM=android-9
TOOLCHAIN_PREFIX=arm-linux-androideabi-
GCC_VER=4.8
TOOLCHAIN_STAMP="$TOOLCHAIN_PATH/.stamp"
[ ! -f "$TOOLCHAIN_STAMP" ] && { \
    "$ANDROID_NDK"/build/tools/make-standalone-toolchain.sh \
        --install-dir="$TOOLCHAIN_PATH" \
        --platform="$ANDROID_PLATFORM" \
        --toolchain="$TOOLCHAIN_PREFIX$GCC_VER" \
        --stl=stlport && \
    touch "$TOOLCHAIN_STAMP"; }
export PATH="$TOOLCHAIN_PATH/bin:$PATH"
export CROSS_SYSROOT="$TOOLCHAIN_PATH/sysroot"
echo -e "Done\n"

TARBALLS_DIR="$ABS_DIR/tarballs"
CONTRIB_SRC_DIR="$ABS_DIR/contrib-${TOOLCHAIN_PREFIX%-}"
INSTALL_DIR="$ABS_DIR/install"

NO_OUTPUT="1>/dev/null 2>&1"

function exit_msg() {
    echo $@
    exit 1
}

TARS_HDLR_ARR=(
"openssl-1.0.2g.tar.gz:compile_openssl"
"fdk-aac-0.1.4.zip:compile_fdk-aac"
"rtmpdump.tar.gz:compile_rtmpdump"
"x264.zip:compile_x264"
)

[ ! -d "$TARBALLS_DIR" ] && { \
    echo "No tarballs dir found"; return 1; }
[ ! -d "$CONTRIB_SRC_DIR" ] && mkdir "$CONTRIB_SRC_DIR"

function compile_openssl() {
    ./Configure android-armv7 zlib-dynamic no-shared --prefix="$INSTALL_DIR" --cross-compile-prefix="$TOOLCHAIN_PREFIX" &&
        make depend &&
        make $MKFLAGS &&
        make install && return 0
    return 1
}

function compile_rtmpdump() {
    sed -i 's/=-lpthread\>/=-pthread/g' Makefile
    make $MKFLAGS CROSS_COMPILE="$TOOLCHAIN_PREFIX" \
         INC=-I"$INSTALL_DIR/include" XLDFLAGS=-L"$INSTALL_DIR"/lib \
         prefix="$INSTALL_DIR" \
         install && return 0
    return 1
}

function compile_fdk-aac() {
    ./configure --host="${TOOLCHAIN_PREFIX%-}" --prefix="$INSTALL_DIR" --enable-static=yes --enable-shared=no && \
        make $MKFLAGS &&
        make install && return 0
    return 1
}

function compile_x264() {
    ./configure --prefix="$INSTALL_DIR" --host=arm-linux --cross-prefix="$TOOLCHAIN_PATH/bin/$TOOLCHAIN_PREFIX" --sysroot="$CROSS_SYSROOT" --enable-static --enable-pic --enable-strip --disable-lavf --disable-avs --disable-swscale --extra-cflags="-DANDROID -march=armv7-a -mtune=cortex-a8 -mfloat-abi=softfp -mfpu=neon -Wno-psabi -msoft-float -Os -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64" && \
        make $MKFLAGS && \
        make install && return 0
    return 1
}

function extract() {
    local tar="$1" bn=`basename "$tar"`
    local dst_parent="$2"
    local tar_suffix_hdlr=(
            ".tar:tar_xvf_@_-C"
            ".tar.gz|.tgz:tar_zxvf_@_-C"
            ".tar.bz|.tar.bz2:tar_jxvf_@_-C"
            ".tar.Z:tar_Zxvf_@_-C"
            ".zip:unzip_-e_@_-d"
            )
    for tsh in ${tar_suffix_hdlr[@]}; do
        local suffix=`echo $tsh | cut -d: -f1`
        local suffix_arr=(${suffix//|/ })
        for sf in ${suffix_arr[@]}; do
            local fm=`echo $bn | sed -n "s/\(.*\)$sf$/\1/p"`
            [ -z "$fm" ] && continue
            local dstsrc="$dst_parent/$fm"
            [ -d  "$dstsrc" ] && \
                { echo "$dstsrc" && return 0; }
            local hdlr=`echo $tsh | cut -d: -f2 | tr -s '_' ' ' | sed -n 's/ @ / $tar /p'`" $dst_parent"
            eval "$hdlr $NO_OUTPUT" || \
                { echo "" && return 1; }
            echo "$dstsrc"
            return 0
        done
    done
}

function compile() {
    local tar="$1"
    local hdlr="$2"
    local dstsrc=`extract "$tar" "$CONTRIB_SRC_DIR"`
    [ -n "$dstsrc" ] || \
        exit_msg "extract \"$tar\" failed"
    cd "$dstsrc"
    local stamp=".stamp"
    [ -f "$stamp" ] && return 0
    $hdlr || \
        exit_msg "compile `basename $tar` failed"
    cd "$dstsrc"
    touch "$stamp"
}

for th in ${TARS_HDLR_ARR[@]}; do
    tar="$TARBALLS_DIR/`echo "$th" | cut -d: -f1`"
    hdlr=`echo "$th" | cut -d: -f2`
    echo "----------------------------"
    echo "[*] $hdlr"
    echo "----------------------------"
    compile "$tar"  "$hdlr"
    echo -e "Done\n"
done

exit 0
