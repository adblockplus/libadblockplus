#!/usr/bin/env bash

set -x
set -e

if [[ ! -d "third_party/prebuilt-v8/include" ]]; then
  wget ${URL_PREFIX}include.7z -O third_party/v8-include.7z
  7z x third_party/v8-include.7z -othird_party/prebuilt-v8
fi
