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

You need Microsoft Visual C++ (Express is sufficient) 2012
and Python 2.6.

- Execute *buildmsvs.bat* from Visual Studio command line (with VS environment variables defined, ie msbuild can be run from)

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

Restrictions on .gyp files
--------------------------

The `msvs` generator is broken; well not so much broken as twisted.
Its behavior induces some restrictions on the way we're able to use `gyp`.
For the full story, see the extended `gyp` documentation.

* All our own command line tools have to have arguments that 
    either (a) begin with a hyphen or slash, 
    or (b) are path names. Sounds bizarre? I'm with you.
* We can't support spaces in path or file names. 
    Strictly speaking, that's only for some path names in certain situations. 
    But please, make everyone's lives easier. 
    Just don't do it.