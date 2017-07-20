V8_DIR :=$(shell pwd -L)/third_party/v8/
HOST_ARCH :=$(shell python ${V8_DIR}gypfiles/detect_v8_host_arch.py)

GYP_PARAMETERS=host_arch=${HOST_ARCH}

ifndef HOST_OS
  raw_OS = $(shell uname -s)
  ifeq (${raw_OS},Linux)
    HOST_OS=linux
  else ifeq (${raw_OS},Darwin)
    HOST_OS=mac
  endif
endif

ifneq ($(ANDROID_ARCH),)
GYP_PARAMETERS+= OS=android target_arch=${ANDROID_ARCH}
ifeq ($(ANDROID_ARCH),arm)
ANDROID_ABI = armeabi-v7a
else ifeq ($(ANDROID_ARCH),ia32)
ANDROID_ABI = x86
else
$(error "Unsupported Android architecture: $(ANDROID_ARCH))
endif
ANDROID_DEST_DIR = android_$(ANDROID_ARCH).release
else
TARGET_ARCH=${HOST_ARCH}
ifdef ARCH
TARGET_ARCH=${ARCH}
endif
GYP_PARAMETERS+= OS=${HOST_OS} target_arch=${TARGET_ARCH}
endif


TEST_EXECUTABLE = build/out/Debug/tests

.PHONY: all test clean docs v8 v8_android_multi android_multi android_x86 \
	android_arm ensure_dependencies

.DEFAULT_GOAL:=all

ensure_dependencies:
	python ensure_dependencies.py

v8: ensure_dependencies
	GYP_DEFINES="${GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I build-v8.gypi --generator-output=build/v8 ${V8_DIR}src/v8.gyp
	make -C build/v8 v8_snapshot v8_libplatform v8_libsampler

all: v8
	GYP_DEFINES="${GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I libadblockplus.gypi --generator-output=build libadblockplus.gyp
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
v8_android_multi: ensure_dependencies
	cd third_party/v8 && GYP_GENERATORS=make-android \
	  GYP_DEFINES="${GYP_PARAMETERS} v8_target_arch=${ANDROID_ARCH}" \
	  PYTHONPATH="${V8_DIR}tools/generate_shim_headers:${V8_DIR}gypfiles:${PYTHONPATH}" \
	  tools/gyp/gyp \
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

android_multi: v8_android_multi_${HOST_OS}_${ANDROID_ARCH}
	GYP_DEFINES="${GYP_PARAMETERS}" \
	python ./make_gyp_wrapper.py --depth=. -f make-android -Ilibadblockplus.gypi --generator-output=build -Gandroid_ndk_version=r9 libadblockplus.gyp
	$(ANDROID_NDK_ROOT)/ndk-build -C build installed_modules \
	BUILDTYPE=Release \
	APP_ABI=$(ANDROID_ABI) \
	APP_PLATFORM=android-9 \
	APP_STL=c++_static \
	APP_BUILD_SCRIPT=Makefile \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	NDK_APP_DST_DIR=$(ANDROID_DEST_DIR)
endif

