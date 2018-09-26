@echo off

pushd %~dp0

python ensure_dependencies.py

mkdir build\ia32\shell
mkdir build\x64\shell
python msvs_gyp_wrapper.py --depth=build\ia32 -f msvs -G msvs_version=2017 -I libadblockplus.gypi --generator-output=build\ia32 -Dtarget_arch=ia32 -Dhost_arch=ia32 abpshell.gyp tests.gyp
python msvs_gyp_wrapper.py --depth=build\x64 -f msvs -G msvs_version=2017 -I libadblockplus.gypi --generator-output=build\x64 -Dtarget_arch=x64 -Dhost_arch=x64 abpshell.gyp tests.gyp
popd
