@echo off
SETLOCAL

rem %1 - MSBuildBinPath, e.g. c:\Program Files (x86)\MSBuild\14.0\Bin
rem %2 - arch {ia32, x64}
rem %3 - configuration {Release, Debug}
set MsBuildBinPath=%~1
set ARCH=%~2
set CONFIGURATION=%~3

pushd "%~dp0"

@python msvs_gyp_wrapper.py --depth=build\%ARCH%\v8 -f msvs -I build-v8.gypi --generator-output=build\%ARCH%\v8 -G msvs_version=2017 -Dtarget_arch=%ARCH% -Dhost_arch=%ARCH% third_party/v8/src/v8.gyp

@"%MSBuildBinPath%/msbuild.exe" /m build/%ARCH%/v8/third_party/v8/src/v8.sln /p:Configuration=%CONFIGURATION% /target:v8_snapshot,v8_libplatform,v8_libsampler

popd

ENDLOCAL

exit /b
