This folder contains files with records of ABP Chromium actions when browsing some sites. This data is used for testing, the details available in [HarnessTest.cpp](../test/HarnessTest.cpp). The sites are listed in the [sites.txt](sites.txt).

You can use the [update.sh](update.sh) script to update the data. Before running the script, you have to:

* Build the ABP Chromium release with the `abp_enable_trace=true` flag
* Set the `CHROME_SRC` environment variable with the root folder of ABP Chromium
* Connect the device
