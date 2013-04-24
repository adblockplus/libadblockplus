@echo off

if %1.==. goto NoArch

pushd %~dp0
mkdir build
mkdir build\shell
python msvs_gyp_wrapper.py --depth=. -f msvs -I common.gypi --generator-output=build -G msvs_version=2012 -Dtarget_arch=%1 libadblockplus.gyp
goto End

:NoArch
  echo Please add a command line parameter specifying target architecture (ia32 or x64)
  goto End

:End
