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

This will build a version of libdabblockplus that uses stubs instead
of the actual Adblock Plus code, because the latter does not work
yet. To build it anyway:

    make test CXXFLAGS="-DFILTER_ENGINE_STUBS=0"

### Windows

You need Microsoft Visual C++ (Express is sufficient) 2010 or later
and Python 2.6.

1. Execute *buildmsvs.bat*
2. Open the solution *MSVS\libadblockplus.sln*
3. Unload the following projects:
 - tests
 - third\_party/googletest
 - third\_party/googletest/gtest_main
4. Disable custom build tools for the following files (right click,
   properties, set *Excluded from Build* to *Yes*):
 - third\_party/v8/tools/gyp/js2c/js2c.py
 - third\_party/v8/tools/gyp/v8\_snapshot/mksnapshot.exe
5. Build the solution (it will fail)
6. Execute *msvs_makesnapshot.bat*
7. Rebuild the solution

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
