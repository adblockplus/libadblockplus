#!/usr/bin/env bash

export ANDROID_NDK_ROOT="$(pwd)/third_party/android-ndk-r16b"
export V8_COMMIT=$(grep -m1 'V8_COMMIT' Makefile | cut -f2 -d\")
export URL_PREFIX="https://v8.eyeofiles.com/v8-$V8_COMMIT/"
build-scripts/android-script.sh
