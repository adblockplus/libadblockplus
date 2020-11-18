libadblockplus
==============

A C++ library offering the core functionality of Adblock Plus.

Git hooks
----------

This repo uses [pre-commit](https://pre-commit.com) to maintain agreed conventions in the repo. It should
be [installed](https://pre-commit.com/#installation) (tldr; `pip install pre-commit` then `pre-commit install`)
before making any new commits to the repo.

Getting/updating the dependencies
---------------------------------

Project dependencies are declared as git submodules, ensure you include submodules
in your checkout.

Additionally one should provide V8 headers in order to build libadblockplus
library and V8 prebuilt libraries in order to link a binary (executable, shared
object/DLL), even libadblockplus tests. The last time is was tested against V8
6.7.
For more details see below.

Building
--------

### Supported target platforms and prerequisites

General:
* You need a C++14 compatible compiler to build libadblockplus. Below there is
the list of tested tools.

* 7z
We use 7z for prebuilt V8. In order to install it on a debian-based system
one can use `p7zip-full` package, or `p7zip-plugins` on rpm-based.

Win32:
* At least v141 Visual C++ toolset (available in Microsoft Visual Studio 2017).

Linux:
* clang 5.0
We use libc++ instead of the libstdc++ that gcc uses, since by default v8 build
with libc++. Make sure you have the right development package installed for
libc++: `libc++-dev` and `libc++abi-dev` on Debian/Ubuntu,
`libcxx-devel` and `libcxxabi-devel` on RedHat/Fedora.

Mac:
* Apple LLVM 9.0.0 for OS X/macOS (Xcode should be installed and its
  developer tools should be "selected").

Android:
* The host system should be Linux or OS X (for Windows 10 [see below](#build-for-android-on-windows))
* android-ndk-r16b, here are the links for downloading
  [OS X](https://dl.google.com/android/repository/android-ndk-r16b-darwin-x86_64.zip),
  [Linux 64](https://dl.google.com/android/repository/android-ndk-r16b-linux-x86_64.zip).
* g++ multilib

If you have a compilation issue with another compiler please [create an issue](https://issues.adblockplus.org/).

### Unix

You need V8 prior to building. Two options:

* Use the default prebuild V8 by invoking the make target
`get-prebuilt-v8`. This will download and extract the prebuilt V8 for
your setup. The default environment will be set by the Makefile at
build time. If you are cross compiling use the same options as below
to invoke make.

Pass `WGET_QUIET=true` to download the files silently.

If you switch the version of V8 using the same source tree, you should
manually remove the `third_party/prebuilt-v8` (also,
`third_party/v8-include.tar.xz` and `third_party/v8-prebuilt.tar.xz` are not
needed) directory and redownload again with the right options.

Or

* Prepare V8 and set environment variables LIBV8_LIB_DIR and LIBV8_INCLUDE_DIR.
LIBV8_INCLUDE_DIR should point to the include directory of V8, e.g.
`.../v8/include` and there should be `libv8_monolith.a` in the directory
LIBV8_LIB_DIR.

To clone:

git clone --recursive https://gitlab.com/eyeo/adblockplus/libadblockplus

To build:

Using Make:

    make

The default target architecture is the architecture of a host. In order to build for a different architecture pass `ABP_TARGET_ARCH` to `make`, e.g. run:

    make ABP_TARGET_ARCH=ia32

supported values are `ia32` and `x64`.

To build and run the tests:

    make test

Likewise, use the following with `ABP_TARGET_ARCH`:

    make test ABP_TARGET_ARCH=ia32

To run specific tests, you can specify a filter:

    make test FILTER=*.Matches

### Windows

* Prepare V8. Let's say V8 is prepared in `build/v8`. There should be V8
headers in `build/v8/include` and binaries in
`build/v8/win-%PLATFORM%.%CONFIGURATION%`, e.g ensure that there is
`v8_monolith.lib` available as `build/v8/win-x64.release/v8_monolith.lib`.

* Set GYP variable `v8_dir` pointing to the prepared V8, `<path to build/v8>`.
E.g. `set "GYP_DEFINES=v8_dir=e:/v8-6.7"` and execute `createsolution.bat` to
generate project files, this will create `build\ia32\libadblockplus.sln`
(solution for the 32 bit build) and `build\x64\libadblockplus.sln` (solution
for the 64 bit build).

* Open `build\ia32\libadblockplus.sln` or `build\x64\libadblockplus.sln` in
Visual Studio and build the solution there. Alternatively you can use the
`msbuild` command line tool, e.g. run `msbuild /m build\ia32\libadblockplus.sln`
from the Visual Studio Developer Command Prompt to create a 32 bit debug build.
Due to linking errors with precompiled V8 binary we have disabled iterator
checking by setting `_ITERATOR_DEBUG_LEVEL=0` in libadblockplus.gypi.

Tested on Microsoft Visual Studio 2017 Community Edition.

For more details see CI configuration for appveyor.

### Building for Android on *nix

Configure V8 as for Unix and set ANDROID_NDK_ROOT environment variable to your
Android NDK directory.

To build for *x86* or *x64* arch run:

    make TARGET_OS=android ABP_TARGET_ARCH=ia32

or

    make TARGET_OS=android ABP_TARGET_ARCH=x64

To build for *arm* or *arm64* arch run:

    make TARGET_OS=android ABP_TARGET_ARCH=arm

or

    make TARGET_OS=android ABP_TARGET_ARCH=arm64

### Build for Android on Windows

Build for Android can be performed in Windows 10 with WSL2 or with docker.
Here we give instructions for WSL 2, feel free to add docker instructions.

* Please, check [how to install WSL 2](https://docs.microsoft.com/en-us/windows/wsl/wsl2-install).
* After installing,
    * open PowerShell (or cmd)
    * navigate to libadblockplus root
    * run (once) the provision script: `wsl sudo ./build-scripts/provision.sh`.
        * run it **only once** in order to prepare environment, no need to run it every time
    * run build script `wsl ./build-scripts/build-android.sh`
You might also run it from inside the WSL2, then do not prefix your commands with `wsl`.

Usage
-----

You can use libadblockplus to build an ad blocker. Or, strictly speaking, a web
content filter. Just like Adblock Plus, it can detect resources that should be
blocked based on their URL and context information, and generate CSS selectors
to hide DOM elements.

The basic usage is explained below, see comments in the header files for more
information. See the [filter documentation](https://adblockplus.org/en/filters)
to learn more about Adblock Plus filters.

### Initialising the engine

All the types and functions in libadblockplus are in the `AdblockPlus`
namespace. For brevity's sake, we'll assume the following `using` declaration:

    using namespace AdblockPlus;

Most of the functionality of libadblockplus is available via the
[`IFilterEngine`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L191)
class. Since libadblockplus uses the Adblock Plus core code under the hood, you
first need to create a
[`JsEngine`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/JsEngine.h#L73)
instance and pass some information about your
application to it.

    AppInfo appInfo;
    appInfo.name = "awesomewebfilter";
    appInfo.version = "0.1";
    appInfo.locale = "en-US";

    auto platform = AdblockPlus::DefaultPlatformBuilder().CreatePlatform();
    platform->SetUpJsEngine(appInfo);
    JsEngine& jsEngine = platform->GetJsEngine();

`JsEngine` needs to store files, make web requests and write log messages. Default implementations
are created using
[`DefaultPlatformBuilder`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/Platform.h#L152).

Depending on your application and platform, you might want to supply your own
implementations. For this, supply `Platform::CreationParameters`
for [`Platform`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/Platform.h#L48) constructor.

With the `JsEngine` instance created, you can create a `IFilterEngine` instance:

    auto filterEngine = IFilterEngine::Create(jsEngine);

Please also pay attention to asynchronous version of factory method
FilterEngineFactory::CreateAsync and to optional creationParameters.

When initialised, `IFilterEngine` will automatically select a suitable ad
blocking subscription based on `AppInfo::locale` and download the filters for
it.

### Managing subscriptions

libadblockplus takes care of storing and updating subscriptions.

You can add more:

    Subscription subscription =
      filterEngine.GetSubscription("https://example.org/filters.txt");
    filterEngine.AddSubscription(subscription);

To retrieve a list of all subscriptions use
[`FilterEngine::GetListedSubscriptions`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L138).
To check if a subscription has been added or not, do the following:

    auto subscriptions = GetFilterEngine().GetListedSubscriptions();
    auto subscription = std::find_if(subscriptions.begin(), subscriptions.end(), [](const auto& cur) {
      return cur.GetUrl() == "https://example.org/filters.txt";
    });
    if (subscription != subscriptions.end())
        ....

Removing a subscription is not rocket science either:

    Subscription subscription =
      filterEngine.GetSubscription("https://example.org/filters.txt");
    filterEngine.RemoveSubscription(subscription);

### Managing custom filters

Working with custom filters is very similar to working with subscriptions:

    Filterr filter = filterEngine.GetFilter("||example.com/ad.png");
    filterEngine.AddFilter(filter);
    filterEngine.RemoveFilter(filter);

Note that applications should only do this to manage a user's custom filters. In
general, filter lists should be hosted somewhere and added as a subscription.

### Matching blocking filters

As mentioned above, one of the two main tasks of libadblockplus is to check if
a URL matches any of the active blocking filters.

To demonstrate this, we'll add a custom filter:

    Filter filter = filterEngine.GetFilter("||example.com/ad.png");
    filterEngine.AddFilter(filter);

Now we'll call matches on an URL that should be blocked:

    Filter match =
      filterEngine.Matches("http://example.com/ad.png", "DOCUMENT", "");

Since we've added a matching filter, `match` will point to the same filter
object as `filter`.

Note that we've ignored the third parameter of
[`IFilterEngine::Matches`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L430)
here to keep things simple. Real applications should pass the frame structure
in here - this is necessary because many filters and exception rules are domain
specific.

### Working with sitekey-restricted filters

Some filters should be applied only for sites providing special key. It is
provided in `X-Adblock-Key` header. Usually it is used to whitelist specific
sites. You can read more about this in
[filters documentation](https://help.eyeo.com/adblockplus/how-to-write-filters#sitekey-restrictions).

To find match filter taking into account site key, please use 4th parameter
for [`IFilterEngine::Matches`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L430)
It should contain decoded and verified public key extracted from `X-Adblock-Key` header.

    Filter match =
      filterEngine.Matches("http://example.com/ad.png", "DOCUMENT", "",
         "DECODED PUBLIC KEY");

You can [take a look](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/test/IFilterEngine.cpp#L668) for the sitekey-related tests for reference.

### Generating CSS from element hiding filters

Aside from blocking requests, ad blockers typically also hide elements. This is
done via a second type of filter that is completely ignored when matching URLs:
[element hiding rules](https://adblockplus.org/en/filters#elemhide).

You can retrieve a CSS style sheet for elements that should be hidden
using
[`IFilterEngine::GetElementHidingStyleSheet`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L493).

What libadblockplus clients typically do with this is to inject the CSS style
sheet into each page.

### Disabling network requests from Adblock Plus on current connection

At any moment you can call [`IFilterEngine::SetAllowedConnectionType`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L541) to change the settings indicating what connection types are allowed in your application. However to have it working you should also pass a callback function into factory method of IFilterEngine. This callback is being called before each request and the value of argument is earlier passed string into [`IFilterEngine::SetAllowedConnectionType`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L541), what allows to query the system and check whether the current connection is in accordance with earlier stored value in settings.
For example, you can pass "not_metered" into [`IFilterEngine::SetAllowedConnectionType`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L541) and on each request you can check whether the current connection is "not_metered" and return true or false from you implementation of callback [`AdblockPlus::IFilterEngine::IsConnectionAllowedAsyncCallback`](https://gitlab.com/eyeo/adblockplus/libadblockplus/blob/master/include/AdblockPlus/IFilterEngine.h#L262).

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

Building V8
-------------------------

Just in case one can find args files to build V8 in `v8-args` directory.

Linting
-------

You can lint the code using [ESLint](http://eslint.org).

    npm run eslint

In order to set up ESLint and
[configuration eslint-config-eyeo](https://gitlab.com/eyeo/auxiliary/eyeo-coding-style/tree/master/eslint-config-eyeo) you need [Node.js 7 or higher](https://nodejs.org/) and once it is installed please run `npm install` in the repository directory.

In order to learn about the usage of deprecated V8 API please set `libv8_show_warnings` to `"true"` on *nix, e.g.

    make libv8_show_warnings="true"

or on Windows add it to `GYP_DEFINES`.
