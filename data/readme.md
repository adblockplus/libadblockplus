# Measuring performance

This folder contains recordings of ABP Chromium actions when browsing sites listed in [sites.txt](sites.txt). They are inputs to performance tests implemented in [HarnessTest.cpp](../test/HarnessTest.cpp).

## Measuring the effect of optimizations

This data can be used to measure the effect of optimizations as follows. It is worth measuring the average time of procedures on the platform on which you are going to test, before applying any code changes.

Measurements should be taken against the release build:

```bash
make Configuration=release get-prebuilt-v8 # If you do not have release V8, only needed once
make Configuration=release FILTER=HarnessTest.AllSites test
```

You should see an output like this:

```
Name                 ; Median(us) ;    Max(us) ;    Min(us) ;      Count
check-filter-match   ;    102.000 ;   3431.000 ;     66.000 ;       4339
generate-js-css      ;    247.000 ;   4566.000 ;     47.000 ;        388
```

After that, you must run the same test for the modified source code. The difference between the measurements will help estimate the effect of tweaks.

**Note:** If the code changes have been applied to the ABP core library, you first need to update that dependency to the desired commit:

```bash
cd adblockpluscore
git checkout <adblockpluscore_commit>
cd ..
```

## Updating trace data

Use the [update.sh](update.sh) script to update the trace data:

* Build the ABP Chromium release with the `abp_enable_trace=true` flag
* Set the `CHROME_SRC` environment variable with the root folder of ABP Chromium
* Connect the device
* Run ./update.sh

