#!/usr/bin/env bash

echo "NOTE: Must be run privileged (root). Run with sudo"

apt-get update && apt-get install -yyq build-essential python wget clang libc++-dev libc++abi-dev unzip pv || exit 1
chmod u+x build-scripts/nix-script.sh
chmod u+x build-scripts/android-script.sh

NDK_ARCHIVE_OUTPUT_DIR=third_party/
NDK_ARCHIVE_FILENAME=android-ndk.zip
NDK_ARCHIVE_PATH=$NDK_ARCHIVE_OUTPUT_DIR$NDK_ARCHIVE_FILENAME
TARGET_FILE_EXISTED=0

# Returns 0 if the downloading step was performed.
# 1 in case the archive was already there and download was skipped.
function DownloadNDK() {
    # It looks like the target file exists (most likely due to a fact this has alreay been run)
    # Let's no download this ~1GB archive again in this case if possible.
    # Unfortunately the file still may be corrupted and there no way to validate it
    # (e.g. no SHA256 file for it possible to be downloaded separately).
	# Let's take the chance and will recover later.

    if [ ! -f $NDK_ARCHIVE_PATH ]; then
	    wget https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip -O $NDK_ARCHIVE_PATH
        return 0
    fi

	return 1
}

function ExtractNDK() {
    echo "Extracting. This may take a while."

    local N_FILES=`unzip -l $NDK_ARCHIVE_PATH | wc -l`
    unzip -o $NDK_ARCHIVE_PATH -d $NDK_ARCHIVE_OUTPUT_DIR | pv -p > /dev/null
    local RESULT=${PIPESTATUS[0]}
    echo "Done."
    return $RESULT
}

DownloadNDK
TARGET_FILE_EXISTED=$?
ExtractNDK
EXTRACTION_STATUS=$?

if [ $EXTRACTION_STATUS -ne 0 ] && [ $TARGET_FILE_EXISTED -eq 1 ]; then
    # It looks like the decision to not to download the archive didn't pan out well. Recover.
    echo "Extracting has failed. The possible reason is the NDK archive being corrupted. Retrying."
    rm $NDK_ARCHIVE_PATH
    DownloadNDK
    ExtractNDK
fi

rm $NDK_ARCHIVE_PATH
