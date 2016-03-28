#! /usr/bin/env bash
set -e

ABS_DIR="$(cd "$(dirname "$0")"; pwd)"
cd "$ABS_DIR"

"$ANDROID_NDK"/ndk-build NDK_DEBUG=1 V=1
ant debug
