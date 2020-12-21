#!/usr/bin/env bash

set -x
set -e

if [[ ! -d "third_party/prebuilt-v8/include" ]]; then
  wget ${WGET_FLAGS} ${URL_PREFIX}/include.tar.xz -O third_party/v8-include.tar.xz
  mkdir -p third_party/prebuilt-v8/
  tar -xJf third_party/v8-include.tar.xz -C third_party/prebuilt-v8
fi
