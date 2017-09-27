@echo off

if "%Platform%"=="Win32" set arch="ia32"

if "%Platform%"=="x64" set arch="x64"

appveyor DownloadFile %URL_PREFIX%win32-%arch%-%Configuration%.7z -FileName prebuilt-v8.7z
appveyor DownloadFile %URL_PREFIX%include.7z -FileName include.7z

7z x include.7z -othird_party/prebuilt-v8
7z x prebuilt-v8.7z -othird_party/prebuilt-v8
