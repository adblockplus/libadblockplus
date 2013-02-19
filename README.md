libadblockplus
==============

A C++ library offering the core functionality of Adblock Plus.

libadblockplus is still work in progress, at an early stage.

Building
--------

### Unix

All you need is Python 2.6 and Make:

    make

To build and run the tests:

    make test

### Windows

TODO: We need to describe how to set up a MSVC project.

Shell
-----

The _shell_ subdirectory contains an example application using libadblockplus.

It's a simple shell that loads subscriptions into memory and checks
whether a specified resource would be blocked or not.

### Unix

The shell is automatically built by `make`, you can run it as follows:

    build/out/abpshell

To see the available commands, type `help`.

### Windows

TODO: We need to describe how to best build and use abpshell in
Windows.
