ARCH := x64

ANDROID_PARAMETERS = OS=android
ifneq ($(ANDROID_ARCH),)
ifeq ($(ANDROID_ARCH),arm)
ANDROID_PARAMETERS += target_arch=arm android_target_arch=arm
ANDROID_PARAMETERS += arm_neon=0 armv7=0 arm_fpu=off vfp3=off
ANDROID_PARAMETERS += arm_float_abi=default
ANDROID_ABI = armeabi-v7a
else ifeq ($(ANDROID_ARCH),ia32)
ANDROID_PARAMETERS += target_arch=x86 android_target_arch=x86
ANDROID_ABI = x86
else
$(error "Unsupported Android architecture: $(ANDROID_ARCH))
endif
ANDROID_DEST_DIR = android_$(ANDROID_ARCH).release
endif

TEST_EXECUTABLE = build/out/Debug/tests

.PHONY: all test clean docs v8_android_multi android_multi android_x86 \
	android_arm

all:
	third_party/gyp/gyp --depth=. -f make -I common.gypi --generator-output=build -Dtarget_arch=$(ARCH) libadblockplus.gyp
	$(MAKE) -C build

test: all
ifdef FILTER
	$(TEST_EXECUTABLE) --gtest_filter=$(FILTER)
else
	$(TEST_EXECUTABLE)
endif

docs:
	doxygen

clean:
	$(RM) -r build docs

android_x86:
	ANDROID_ARCH="ia32" $(MAKE) android_multi

android_arm:
	ANDROID_ARCH="arm" $(MAKE) android_multi

ifneq ($(ANDROID_ARCH),)
v8_android_multi:
	mkdir -p third_party/v8/build/gyp
	cp -f third_party/v8_gyp_launcher third_party/v8/build/gyp/gyp
	DEFINES="${ANDROID_PARAMETERS}" \
	OUTDIR=../../build \
	$(MAKE) -C third_party/v8 $(ANDROID_DEST_DIR)

android_multi: v8_android_multi
	GYP_DEFINES="${ANDROID_PARAMETERS} ANDROID_ARCH=$(ANDROID_ARCH)" \
	third_party/gyp/gyp --depth=. -f make-android -I common.gypi --generator-output=build -Gandroid_ndk_version=r9 libadblockplus.gyp
	$(ANDROID_NDK_ROOT)/ndk-build -C build installed_modules \
	BUILDTYPE=Release \
	APP_ABI=$(ANDROID_ABI) \
	APP_PLATFORM=android-9 \
	APP_STL=c++_static \
	APP_BUILD_SCRIPT=Makefile \
	NDK_TOOLCHAIN_VERSION=clang3.4 \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	NDK_APP_DST_DIR=$(ANDROID_DEST_DIR)
endif

