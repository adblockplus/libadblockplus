@echo off
setlocal
pushd %~dp0
set ARCH=x86     

rem Verify that the python interpreter is available
if not defined PYTHON (set PYTHON=python)
%PYTHON% -c "exit 0" 2>nul
if not errorlevel 9009 goto pythonOK
echo Error. Cannot locate Python interpreter. 
set spaces=    
echo %spaces%PYTHON environment variable must point to a Python 2.7 interpreter.     
echo %spaces%It may be either root name that appears in the path or the fully qualified name of an executable.
echo %spaces%If the variable is absent, it defaults to "python".
echo %spaces%PYTHON=%PYTHON%
exit /b 1
:pythonOK

cls
@echo on

rem There's no way to set generator flags within the .gyp file, so we have to set them here.
set GYP_GENERATOR_FLAGS=msvs_list_excluded_files=0
%PYTHON% third_party\gyp\gyp --help
%PYTHON% third_party\gyp\gyp --depth=src -f msvs -I common.gypi --generator-output=build_solo -Dpython=%PYTHON% -Dtarget_arch=%ARCH% libadblockplus-solo.gyp
echo Finished
