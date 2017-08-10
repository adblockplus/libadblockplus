V8_DIR :=$(shell pwd -L)/third_party/v8/
HOST_ARCH :=$(shell python third_party/detect_v8_host_arch.py)

GYP_PARAMETERS=host_arch=${HOST_ARCH}

ifndef HOST_OS
  raw_OS = $(shell uname -s)
  ifeq (${raw_OS},Linux)
    HOST_OS=linux
  else ifeq (${raw_OS},Darwin)
    HOST_OS=mac
  endif
endif

ifneq "$(and ${LIBV8_LIB_DIR}, ${LIBV8_INCLUDE_DIR})" ""
BUILD_V8=do-nothing
ABP_GYP_PARAMETERS+= libv8_lib_dir=${LIBV8_LIB_DIR} libv8_include_dir=${LIBV8_INCLUDE_DIR}
else
BUILD_V8=build-v8
endif

ifneq ($(ANDROID_ARCH),)
ANDROID_PLATFORM_LEVEL=android-9
GYP_PARAMETERS+= OS=android target_arch=${ANDROID_ARCH}
ifeq ($(ANDROID_ARCH),arm)
ANDROID_ABI = armeabi-v7a
else ifeq ($(ANDROID_ARCH),ia32)
ANDROID_ABI = x86
else ifeq ($(ANDROID_ARCH),arm64)
ANDROID_ABI = arm64-v8a
# minimal platform having arch-arm64, see ndk/platforms
ANDROID_PLATFORM_LEVEL=android-21
else
$(error "Unsupported Android architecture: $(ANDROID_ARCH))
endif
ANDROID_DEST_DIR = android_$(ANDROID_ARCH).release

ifeq "$(and ${LIBV8_LIB_DIR}, ${LIBV8_INCLUDE_DIR})" ""
ABP_GYP_PARAMETERS+= libv8_lib_dir=${ANDROID_DEST_DIR}
BUILD_V8=build-v8-android
endif

else # if ${ANDROID_ARCH} is empty
TARGET_ARCH=${HOST_ARCH}
ifdef ARCH
TARGET_ARCH=${ARCH}
endif
GYP_PARAMETERS+= OS=${HOST_OS} target_arch=${TARGET_ARCH}
endif


TEST_EXECUTABLE = build/out/Debug/tests

.PHONY: do-nothing all test clean docs build-v8 build-v8-android v8_android_multi android_multi android_x86 \
	android_arm ensure_dependencies

.DEFAULT_GOAL:=all

do-nothing:

ensure_dependencies:
	python ensure_dependencies.py

build-v8: ensure_dependencies
	GYP_DEFINES="${GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I build-v8.gypi --generator-output=build/v8 ${V8_DIR}src/v8.gyp
	make -C build/v8 v8_snapshot v8_libplatform v8_libsampler

all: ${BUILD_V8} ensure_dependencies 
	GYP_DEFINES="${GYP_PARAMETERS} ${ABP_GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I libadblockplus.gypi --generator-output=build libadblockplus.gyp
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

android_arm64:
	ANDROID_ARCH="arm64" $(MAKE) android_multi

ifneq ($(ANDROID_ARCH),)
v8_android_multi: ensure_dependencies
	cd third_party/v8 && GYP_GENERATORS=make-android \
	  GYP_DEFINES="${GYP_PARAMETERS} v8_target_arch=${ANDROID_ARCH}" \
	  PYTHONPATH="${V8_DIR}tools/generate_shim_headers:${V8_DIR}gypfiles:${PYTHONPATH}" \
	  python ../../make_gyp_wrapper.py \
	    --generator-output=../../build src/v8.gyp \
	    -Igypfiles/standalone.gypi \
	    --depth=. \
	    -S.android_${ANDROID_ARCH}.release \
	    -I../../android-v8-options.gypi
	cd third_party/v8 && make \
	  -C ../../build \
	  -f Makefile.android_${ANDROID_ARCH}.release \
	  v8_snapshot v8_libplatform v8_libsampler \
	  BUILDTYPE=Release \
	  builddir=${V8_DIR}../../build/android_${ANDROID_ARCH}.release

v8_android_multi_linux_${ANDROID_ARCH}: v8_android_multi

v8_android_multi_mac_ia32: v8_android_multi
	find build/android_ia32.release/ -depth 1 -iname \*.a -exec ${ANDROID_NDK_ROOT}/toolchains/x86-4.9/prebuilt/darwin-x86_64/bin/i686-linux-android-ranlib {} \;

v8_android_multi_mac_arm: v8_android_multi
	find build/android_arm.release/ -depth 1 -iname \*.a -exec ${ANDROID_NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ranlib {} \;

v8_android_multi_mac_arm64: v8_android_multi
	find build/android_arm64.release/ -depth 1 -iname \*.a -exec ${ANDROID_NDK_ROOT}/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/bin/aarch64-linux-android-ranlib {} \;

build-v8-android: v8_android_multi_${HOST_OS}_${ANDROID_ARCH}

android_multi: ${BUILD_V8} ensure_dependencies
	GYP_DEFINES="${GYP_PARAMETERS} ${ABP_GYP_PARAMETERS}" \
	python ./make_gyp_wrapper.py --depth=. -f make-android -Ilibadblockplus.gypi --generator-output=build -Gandroid_ndk_version=r9 libadblockplus.gyp
	$(ANDROID_NDK_ROOT)/ndk-build -C build installed_modules \
	BUILDTYPE=Release \
	APP_ABI=$(ANDROID_ABI) \
	APP_PLATFORM=${ANDROID_PLATFORM_LEVEL} \
	APP_PIE=true \
	APP_STL=c++_static \
	APP_BUILD_SCRIPT=Makefile \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	NDK_APP_DST_DIR=$(ANDROID_DEST_DIR)
endif

