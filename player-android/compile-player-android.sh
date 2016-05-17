#! /usr/bin/env bash
# compile-player-android.sh

ABS_DIR="$(cd "$(dirname "$0")"; pwd)"
cd "$ABS_DIR"

ant debug
