#!/usr/bin/env bash

set -x
set -e

wget ${URL_PREFIX}include.7z -O third_party/v8-include.7z
7z x third_party/v8-include.7z -othird_party/prebuilt-v8

if [[ "${BUILD_ACTION}" = "android_arm" ]]; then
PREBUILT_V8_ARCHIVE=android-arm-release.tar.xz
elif [[ "${BUILD_ACTION}" = "android_arm64" ]]; then
PREBUILT_V8_ARCHIVE=android-arm64-release.tar.xz
elif [[ "${BUILD_ACTION}" = "android_x86" ]]; then
PREBUILT_V8_ARCHIVE=android-x86-release.tar.xz
else
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
PREBUILT_V8_ARCHIVE=osx-x64-debug.tar.xz
else
PREBUILT_V8_ARCHIVE=u14.04-x64-debug.tar.xz
fi
fi

wget ${URL_PREFIX}/${PREBUILT_V8_ARCHIVE} -O third_party/v8-prebuilt.tar.xz
tar -xJf third_party/v8-prebuilt.tar.xz -C third_party/prebuilt-v8
