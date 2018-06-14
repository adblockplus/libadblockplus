HOST_ARCH :=$(shell python third_party/detect_v8_host_arch.py)

ifndef HOST_OS
  raw_OS = $(shell uname -s)
  ifeq (${raw_OS},Linux)
    HOST_OS=linux
  else ifeq (${raw_OS},Darwin)
    HOST_OS=mac
  endif
endif

TARGET_OS ?= ${HOST_OS}
TARGET_ARCH ?= ${HOST_ARCH}
Configuration ?= debug

BUILD_DIR ?=$(shell pwd -L)/build

ifeq (${TARGET_OS},android)
Configuration ?= release
ANDROID_PLATFORM_LEVEL=android-16
ANDROID_FIXES=
ifeq ($(TARGET_ARCH),arm)
ANDROID_ABI = armeabi-v7a
ANDROID_FIXES="LOCAL_LDFLAGS=\"-Wl,--allow-multiple-definition\""
else ifeq ($(TARGET_ARCH),ia32)
ANDROID_ABI = x86
else ifeq ($(TARGET_ARCH),arm64)
ANDROID_ABI = arm64-v8a
# minimal platform having arch-arm64, see ndk/platforms
ANDROID_PLATFORM_LEVEL=android-21
else
$(error "Unsupported Android architecture: $(TARGET_ARCH)")
endif
endif

# linux,osx
#   x64, ia32
#     debug, release
# android
#   arm, arm64, ia32
#     release

GYP_PARAMETERS=host_arch=${HOST_ARCH} OS=${TARGET_OS} target_arch=${TARGET_ARCH}
ifneq "$(and ${LIBV8_LIB_DIR}, ${LIBV8_INCLUDE_DIR})" ""
GYP_PARAMETERS+= libv8_lib_dir=${LIBV8_LIB_DIR} libv8_include_dir=${LIBV8_INCLUDE_DIR}
else
$(error "V8 directories are not specified")
endif

TEST_EXECUTABLE = ${BUILD_DIR}/out/Debug/tests

.PHONY: all test clean docs ensure_dependencies

.DEFAULT_GOAL:=all

ensure_dependencies:
	python ensure_dependencies.py

test: all
ifdef FILTER
	$(TEST_EXECUTABLE) --gtest_filter=$(FILTER)
else
	$(TEST_EXECUTABLE)
endif

docs:
	doxygen

clean:
	$(RM) -r ${BUILD_DIR} docs

ifeq ($(TARGET_OS),android)
all: ensure_dependencies
	GYP_DEFINES="${GYP_PARAMETERS}" \
	python ./make_gyp_wrapper.py --depth=. -f make-android -Ilibadblockplus.gypi --generator-output=${BUILD_DIR} -Gandroid_ndk_version=r16b libadblockplus.gyp
	$(ANDROID_NDK_ROOT)/ndk-build -C ${BUILD_DIR} installed_modules \
	BUILDTYPE=Release \
	APP_ABI=$(ANDROID_ABI) \
	APP_PLATFORM=${ANDROID_PLATFORM_LEVEL} \
	APP_PIE=true \
	APP_STL=c++_shared \
	APP_BUILD_SCRIPT=Makefile \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	${ANDROID_FIXES} \
	NDK_APP_DST_DIR=android-$(TARGET_ARCH).release
else
all: ensure_dependencies 
	GYP_DEFINES="${GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I libadblockplus.gypi --generator-output=${BUILD_DIR} libadblockplus.gyp
	$(MAKE) -C ${BUILD_DIR}

endif
