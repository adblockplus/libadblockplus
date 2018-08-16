#!/usr/bin/env bash

set -x
set -e

export Configuration=release
export TARGET_OS=android
ABP_TARGET_ARCHES=("arm" "arm64" "ia32")
for ABP_TARGET_ARCH in "${ABP_TARGET_ARCHES[@]}"
do
  export ABP_TARGET_ARCH=${ABP_TARGET_ARCH}
  make get-prebuilt-v8
  make all
done
