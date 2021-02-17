This folder contains files with records of ABP Chromium actions when browsing some sites. This data is used for testing, the details available in [HarnessTest.cpp](../test/HarnessTest.cpp). The sites are listed in the [sites.txt](sites.txt).

## Measuring the effect of optimizations

This data can be used to measure the effect of optimizations as follows. First, it is worth measuring the average time of procedures before changes for the platform on which you are going to test. It should be done for the release build.

```
make Configuration=release get-prebuilt-v8 # If you do not have release V8
make Configuration=release FILTER=HarnessTest.AllSites test
```

You will have output like this:

```
Name                 ;  Avg, us ;  Max, us
check-filter-match   ;  119.107 ; 3803.000
generate-js-css      ;  306.869 ; 4427.000
```

After that, you must run the same test for the modified source code. The difference between the measurements will help estimate the effect of tweaks.

## Updating data

You can use the [update.sh](update.sh) script to update the data. Before running the script, you have to:

* Build the ABP Chromium release with the `abp_enable_trace=true` flag
* Set the `CHROME_SRC` environment variable with the root folder of ABP Chromium
* Connect the device
