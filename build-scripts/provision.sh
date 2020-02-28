#!/usr/bin/env bash

echo "NOTE: Most be run privileged (root). Run with sudo"

apt-get update
apt-get install -yyq build-essential python wget p7zip-full clang libc++-dev libc++abi-dev
chmod u+x .travis/nix-script.sh
chmod u+x .travis/android-script.sh

wget https://dl.google.com/android/repository/android-ndk-r16b-linux-x86_64.zip -O third_party/android-ndk.zip
unzip -q third_party/android-ndk.zip -d third_party/
rm android-ndk.zip