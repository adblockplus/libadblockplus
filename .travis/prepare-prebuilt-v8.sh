#!/usr/bin/env bash

set -x
set -e

if [[ ! -d "third_party/prebuilt-v8/include" ]]; then
  wget ${URL_PREFIX}include.7z -O third_party/v8-include.7z
  7z x third_party/v8-include.7z -othird_party/prebuilt-v8
fi

PREBUILT_V8_ARCHIVE=${TARGET_OS}-${ABP_TARGET_ARCH}-${Configuration}.tar.xz

if [[ "${TARGET_OS}" != "android" ]]; then
if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
PREBUILT_V8_ARCHIVE=osx-x64-debug.tar.xz
elif [[ "${TRAVIS_OS_NAME}" == "linux" ]]; then
PREBUILT_V8_ARCHIVE=u14.04-x64-debug.tar.xz
fi
fi

wget ${URL_PREFIX}/${PREBUILT_V8_ARCHIVE} -O third_party/v8-prebuilt.tar.xz
mkdir -p third_party/prebuilt-v8/${TARGET_OS}-${ABP_TARGET_ARCH}-${Configuration}
tar -xJf third_party/v8-prebuilt.tar.xz -C third_party/prebuilt-v8/${TARGET_OS}-${ABP_TARGET_ARCH}-${Configuration}
