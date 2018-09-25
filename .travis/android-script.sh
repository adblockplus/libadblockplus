#!/usr/bin/env bash

set -x
set -e

export Configuration=release
export TARGET_OS=android
ABP_TARGET_ARCHES=("arm" "arm64" "ia32")
for ABP_TARGET_ARCH in "${ABP_TARGET_ARCHES[@]}"
do
  export ABP_TARGET_ARCH=${ABP_TARGET_ARCH}
  # ensure that one can build libadblockplus.a without having binaries of V8
  bash .travis/prepare-v8-headers.sh
  make GYP_FILE=libadblockplus.gyp APP_MODULES=adblockplus
  # build tests
  make get-prebuilt-v8
  make
  # build abpshell
  make GYP_FILE=abpshell.gyp
done
