#! /usr/bin/env bash
set -e

if [ -z "$ANDROID_NDK" -o -z "$ANDROID_SDK" ]; then
    echo "You must define ANDROID_NDK, ANDROID_SDK before starting."
    echo "They must point to your NDK and SDK directories."
    exit 1
fi

UNAMES=$(uname -s)
export MKFLAGS=
if which nproc >/dev/null; then
    export MKFLAGS=-j`nproc`
else
    export MKFLAGS=-j`cat /proc/cpuinfo | grep '^processor.*: *[0-9]*' | wc -l`
fi

bash contrib/compile-contrib.sh
bash libfqrtmp/compile-libfqrtmp.sh
