#!/bin/bash

PKG=org.chromium.chrome.stable
ADB=$CHROME_SRC/src/third_party/android_sdk/public/platform-tools/adb

# Install and start release apk
$CHROME_SRC/src/out/Release/bin/chrome_public_apk run --no-logcat

while read -r URL; do
echo $URL
# Clean the log
$ADB shell -n rm /data/user/0/$PKG/cache/adblock_trace.log
# Request $PKG to open url
$ADB shell -n am start -p $PKG                                  \
        -a android.intent.action.VIEW                           \
        -d "$URL"                                               \
        --es com.android.browser.application_id org.adblockplus
# Wait until page is loaded
sleep 20s
# Go to about:blank to stop loading
$ADB shell -n am start -p $PKG                                  \
        -a android.intent.action.VIEW                           \
        -d "about:blank"                                        \
        --es com.android.browser.application_id org.adblockplus
# Pull data from the device
$ADB pull /data/user/0/$PKG/cache/adblock_trace.log             \
        rec_$(echo ${URL//./_} | cut -d '/' -f3).log
done < sites.txt

# Pull blocking rules from the device
$ADB pull /data/user/0/$PKG/cache/patterns.ini /data/user/0/$PKG/cache/prefs.json .
