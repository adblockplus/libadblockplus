#!/usr/bin/env bash

set -x
set -e

make get-prebuilt-v8
make test
