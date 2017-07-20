libadblockplus
==============

A C++ library offering the core functionality of Adblock Plus.

Getting/updating the dependencies
---------------------------------

libadblockplus has dependencies that aren't part of this repository. They are
retrieved and updated during the build process, but you can also manually update
them by running the following:

    ./ensure_dependencies.py

Building
--------

### Supported target platforms and prerequisites

You need a C++11 compatible compiler to build libadblockplus.

Win32:
* At least v140 Visual C++ toolset (available in Microsoft Visual Studio 2015).

Linux:
* g++ 5.2

Mac:
* clang 3.6 for OS X/macOS (Xcode should be installed and its developer tools should be "selected").

Android:
* The host system should be Linux or OS X
* android-ndk-r12b Here are the links for downloading
  [OS X](https://dl.google.com/android/repository/android-ndk-r12b-darwin-x86_64.zip), 
  [Linux 64](https://dl.google.com/android/repository/android-ndk-r12b-linux-x86_64.zip).
* g++ multilib

If you have a compilation issue with another compiler please [create an issue](https://issues.adblockplus.org/).

You also need Python 2.7 and ensure that `python.exe` is in your `PATH`.

### Unix

Using Make:

    make

The default target architecture is the architecture of a host. In order to build for a different architecture pass `ARCH` to `make`, e.g. run:

    make ARCH=ia32

supported values are `ia32` and `x64`.
    

To build and run the tests:

    make test

Likewise, use the following with `ARCH`:

    make test ARCH=ia32

To run specific tests, you can specify a filter:

    make test FILTER=*.Matches

### Windows

* Execute `createsolution.bat` to generate project files, this will create
`build\ia32\libadblockplus.sln` (solution for the 32 bit build) and
`build\x64\libadblockplus.sln` (solution for the 64 bit build). Unfortunately,
V8 doesn't support creating both from the same project files.
* Open `build\ia32\libadblockplus.sln` or `build\x64\libadblockplus.sln` in
Visual Studio and build the solution there. Alternatively you can use the
`msbuild` command line tool, e.g. run `msbuild /m build\ia32\libadblockplus.sln`
from the Visual Studio Developer Command Prompt to create a 32 bit debug build.

Tested on Microsoft Visual Studio 2015 Community Edition.

### Building for Android

First set ANDROID_NDK_ROOT environment variable to your Android NDK directory.

To build for *x86* arch run:

    make android_x86

To build for *arm* arch run:

    make android_arm

Usage
-----

You can use libadblockplus to build an ad blocker. Or, strictly speaking, a web
content filter. Just like Adblock Plus, it can detect resources that should be
blocked based on their URL and context information, and generate CSS selectors
to hide DOM elements.

The basic usage is explained below, see the
[API documentation](https://adblockplus.org/docs/libadblockplus) for more
information. See the [filter documentation](https://adblockplus.org/en/filters)
to learn more about Adblock Plus filters.

### Initialising the engine

All the types and functions in libadblockplus are in the `AdblockPlus`
namespace. For brevity's sake, we'll assume the following `using` declaration:

    using namespace AdblockPlus;

Most of the functionality of libadblockplus is available via the
[`FilterEngine`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_filter_engine.html)
class. Since libadblockplus uses the Adblock Plus core code under the hood, you
first need to create a
[`JsEngine`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_js_engine.html)
instance and pass some information about your
application to it.

    AppInfo appInfo;
    appInfo.name = "awesomewebfilter";
    appInfo.version = "0.1";
    appInfo.locale = "en-US";
    JsEngine jsEngine(appInfo);

`JsEngine` needs to store files, make web requests and write log messages. This
normally works out of the box because it is using
[`DefaultFileSystem`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_default_file_system.html),
[`DefaultWebRequest`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_default_web_request.html)
and
[`DefaultLogSystem`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_default_log_system.html)
by default.

Depending on your application and platform, you might want to supply your own
implementations for these - see
[`FilterEngine::SetFileSystem`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_js_engine.html#a979e9bde78499dab9f5e3aacc5155f40),
[`FilterEngine::SetWebRequest`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_js_engine.html#a290a03b86137a56d7b2f457f03c77504)
and
[`FilterEngine::SetLogSystem`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_js_engine.html#ab60b10be1d4500bce4b17c1e9dbaf4c8)
respectively.

With the `JsEngine` instance created, you can create a `FilterEngine` instance:

    auto filterEngine = FilterEngine::Create(jsEngine);

Please also pay attention to asynchronous version of factory method
FilterEngine::CreateAsync and to optional creationParameters.

When initialised, `FilterEngine` will automatically select a suitable ad
blocking subscription based on `AppInfo::locale` and download the filters for
it.

### Managing subscriptions

libadblockplus takes care of storing and updating subscriptions.

You can add more:

    SubscriptionPtr subscription =
      filterEngine.GetSubscription("https://example.org/filters.txt");
    subscription->AddToList();

Retrieving an existing subscription works the same way, use
[`Subscription::IsListed`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_subscription.html#a42da64bdc0cb7ee65a27a001db0307c8)
to check if the subscription has been added or not.

    SubscriptionPtr subscription =
      filterEngine.GetSubscription("https://example.org/filters.txt");
    if (subscription->IsListed())
        ....

Removing a subscription is not rocket science either:

    subscription->RemoveFromList();

You can also get a list of all subscriptions that were added:

    std::vector<SubscriptionPtr> subscriptions =
      filterEngine.GetListedSubscriptions();

### Managing custom filters

Working with custom filters is very similar to working with subscriptions:

    FilterPtr filter = filterEngine.GetFilter("||example.com/ad.png");
    filter->AddToList();
    filter->RemoveFromList();

Note that applications should only do this to manage a user's custom filters. In
general, filter lists should be hosted somewhere and added as a subscription.

### Matching blocking filters

As mentioned above, one of the two main tasks of libadblockplus is to check if
a URL matches any of the active blocking filters.

To demonstrate this, we'll add a custom filter:

    FilterPtr filter = filterEngine.GetFilter("||example.com/ad.png");
    filter->AddToList();

Now we'll call matches on an URL that should be blocked:

    FilterPtr match =
      filterEngine.Matches("http://example.com/ad.png", "DOCUMENT", "");

Since we've added a matching filter, `match` will point to the same filter
object as `filter`.

Note that we've ignored the third parameter of
[`FilterEngine::Matches`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_filter_engine.html#a184211d324bfb6e23c5e09fae2d12f91)
here to keep things simple. Real applications should pass the frame structure
in here - this is necessary because many filters and exception rules are domain
specific.

### Generating CSS from element hiding filters

Aside from blocking requests, ad blockers typically also hide elements. This is
done via a second type of filter that is completely ignored when matching URLs:
[element hiding rules](https://adblockplus.org/en/filters#elemhide).

You can retrieve a list of all CSS selectors for elements that should be hidden
using
[`FilterEngine::GetElementHidingSelectors`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_filter_engine.html#a91c44dac13c7655230be49440f45a168).

What libadblockplus clients typically do with this is to generate a CSS style
sheet that is injected into each page.

### Disabling network requests from Adblock Plus on current connection
At any moment you can call [`FilterEngine::SetAllowedConnectionType`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_filter_engine.html#a4bee602fb50abcb945d3f19468fd8893) to change the settings indicating what connection types are allowed in your application. However to have it working you should also pass a callback function into factory method of FilterEngine. This callback is being called before each request and the value of argument is earlier passed string into `FilterEngine::SetAllowedConnectionType`, what allows to query the system and check whether the current connection is in accordance with earlier stored value in settings.
For example, you can pass "not_metered" into [`FilterEngine::SetAllowedConnectionType`](https://adblockplus.org/docs/libadblockplus/class_adblock_plus_1_1_filter_engine.html#a4bee602fb50abcb945d3f19468fd8893) and on each request you can check whether the current connection is "not_metered" and return true or false from you implementation of callback [`AdblockPlus::FilterEngine::CreateParameters::isConnectionAllowed`](https://adblockplus.org/docs/libadblockplus/structAdblockPlus_1_1FilterEngine_1_1CreateParameters.html#a86f427300972d3f98bb6d4108301a526).

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

Building with prebuilt V8
-------------------------

This functionality is only for internal usage.

## How to use it:
Let's say that v8 is stored in `libadblockplus/v8-bins`.
### Windows

    set "GYP_DEFINES=libv8_include_dir=../../v8-bins/win/include libv8_lib_dir=../../v8-bins/win/Win32/<(CONFIGURATION_NAME) libv8_no_build=true"
    createsolution.bat

### *nix

    [ANDROID_NDK_ROOT=....] make [android_...] LIBV8_LIB_DIR=../v8-bins/libs LIBV8_INCLUDE_DIR=../v8-bins/include

The rest is the same.
