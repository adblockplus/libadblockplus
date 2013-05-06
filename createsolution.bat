@echo off

pushd %~dp0
mkdir build\ia32\shell
mkdir build\x64\shell
python msvs_gyp_wrapper.py --depth=build\ia32 -f msvs -I common.gypi --generator-output=build\ia32 -G msvs_version=2012 -Dtarget_arch=ia32 libadblockplus.gyp
python msvs_gyp_wrapper.py --depth=build\x64 -f msvs -I common.gypi --generator-output=build\x64 -G msvs_version=2012 -Dtarget_arch=x64 libadblockplus.gyp
