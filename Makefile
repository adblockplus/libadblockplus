ARCH := x64

ANDROID_PARAMETERS = target_arch=arm OS=android
ANDROID_PARAMETERS += android_target_arch=arm
ANDROID_PARAMETERS += arm_neon=0 armv7=0 arm_fpu=off vfp3=off

TEST_EXECUTABLE = build/out/Debug/tests

.PHONY: all test clean v8_android android

all:
	third_party/gyp/gyp --depth=. -f make -I common.gypi --generator-output=build -Dtarget_arch=$(ARCH) libadblockplus.gyp
	$(MAKE) -C build

test: all
ifdef FILTER
	$(TEST_EXECUTABLE) --gtest_filter=$(FILTER)
else
	$(TEST_EXECUTABLE)
endif

clean:
	$(RM) -r build

v8_android:
	mkdir -p third_party/v8/build/gyp
	cp -f third_party/v8_gyp_launcher third_party/v8/build/gyp/gyp
	DEFINES="${ANDROID_PARAMETERS}" \
	OUTDIR=../../build \
	$(MAKE) -C third_party/v8 android_arm.release

android: v8_android
	GYP_DEFINES="${ANDROID_PARAMETERS}" \
	third_party/gyp/gyp --depth=. -f make -I common.gypi --generator-output=build -Gandroid_ndk_version=r9 libadblockplus.gyp
	ndk-build -C build installed_modules \
	BUILDTYPE=Release \
	APP_PLATFORM=android-9 \
	APP_STL=gnustl_static \
	APP_BUILD_SCRIPT=Makefile \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	NDK_APP_DST_DIR=android_arm.release
