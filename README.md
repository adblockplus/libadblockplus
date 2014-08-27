libadblockplus
==============

A C++ library offering the core functionality of Adblock Plus.

libadblockplus is still work in progress, at an early stage.

Building
--------

### Unix

All you need is Python 2.6 and Make:

    make

The default target architecture is x64. On a 32 bit system, run:

    make ARCH=ia32

To build and run the tests:

    make test

Likewise, use the following on a 32 bit system:

    make test ARCH=ia32

To run specific tests, you can specify a filter:

    make test FILTER=*.Matches

### Windows

You need Microsoft Visual C++ (Express is sufficient) 2012
and Python 2.6. Make sure that `python.exe` is on your `PATH`.

* Execute `createsolution.bat` to generate project files, this will create
`build\ia32\libadblockplus.sln` (solution for the 32 bit build) and
`build\x64\libadblockplus.sln` (solution for the 64 bit build). Unfortunately,
V8 doesn't support creating both from the same project files.
* Open `build\ia32\libadblockplus.sln` or `build\x64\libadblockplus.sln` in
Visual Studio and build the solution there. Alternatively you can use the
`msbuild` command line tool, e.g. run `msbuild /m build\ia32\libadblockplus.sln`
from the Visual Studio Developer Command Prompt to create a 32 bit debug build.

Shell
-----

The _shell_ subdirectory contains an example application using libadblockplus.

It's a simple shell that loads subscriptions into memory and checks
whether a specified resource would be blocked or not.

To see the available commands, type `help`.

### Unix

The shell is automatically built by `make`, you can run it as follows:

    build/out/abpshell

### Windows

Just run the project *abpshell*.
