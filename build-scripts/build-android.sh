#!/usr/bin/env bash

export ANDROID_NDK_ROOT="$(pwd)/third_party/android-ndk-r20b"
export V8_COMMIT=$(make get-v8-commit)
export URL_PREFIX="https://v8.eyeofiles.com/v8-$V8_COMMIT/"
build-scripts/android-script.sh
