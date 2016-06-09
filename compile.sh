#! /usr/bin/env bash
# compile.sh

if [ -z "$ANDROID_NDK" -o -z "$ANDROID_SDK" ]; then
    echo "You must define ANDROID_NDK, ANDROID_SDK before starting."
    echo "They must point to your NDK and SDK directories."
    exit 1
fi

if [ -z "$MKFLAGS" ]; then
    UNAMES=$(uname -s)
    MKFLAGS=
    if which nproc >/dev/null; then
        export MKFLAGS=-j`nproc`
    elif [ "$UNAMES" == "Darwin" ] && which sysctl >/dev/null; then
        export MKFLAGS=-j`sysctl -n machdep.cpu.thread_count`
    fi
fi

bash contrib/compile-contrib.sh
bash player-android/compile-player-android.sh
