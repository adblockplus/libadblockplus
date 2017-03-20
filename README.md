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
* Microsoft Visual Studio 2010, 2012

Linux:
* g++ 5.2

Mac:
* clang 3.6 for OS X

Android:
* The host system should be Linux or OS X
* android-ndk-r9, android-ndk-r10c. You can download the latter for [OS X](http://dl.google.com/android/ndk/android-ndk-r10c-darwin-x86_64.bin), [Linux 32](http://dl.google.com/android/ndk/android-ndk-r10c-linux-x86.bin), [Linux 64](http://dl.google.com/android/ndk/android-ndk-r10c-linux-x86_64.bin).
* g++ multilib

If you have a compilation issue with another compiler please [create an issue](https://issues.adblockplus.org/).

### Unix

All you need is Python 2.7 and Make:

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
and Python 2.7. Make sure that `python.exe` is on your `PATH`.

* Execute `createsolution.bat` to generate project files, this will create
`build\ia32\libadblockplus.sln` (solution for the 32 bit build) and
`build\x64\libadblockplus.sln` (solution for the 64 bit build). Unfortunately,
V8 doesn't support creating both from the same project files.
* Open `build\ia32\libadblockplus.sln` or `build\x64\libadblockplus.sln` in
Visual Studio and build the solution there. Alternatively you can use the
`msbuild` command line tool, e.g. run `msbuild /m build\ia32\libadblockplus.sln`
from the Visual Studio Developer Command Prompt to create a 32 bit debug build.

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
