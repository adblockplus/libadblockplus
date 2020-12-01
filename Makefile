HOST_ARCH :=$(shell python build-scripts/detect_v8_host_arch.py)

ifndef HOST_OS
  raw_OS = $(shell uname -s)
  ifeq (${raw_OS},Linux)
    HOST_OS=linux
    TRAVIS_OS_NAME=linux
    ifeq ($(origin CXX),default)
      CXX=clang++
    endif
  else ifeq (${raw_OS},Darwin)
    HOST_OS=mac
    TRAVIS_OS_NAME=osx
  endif
endif

TARGET_OS ?= ${HOST_OS}
ABP_TARGET_ARCH ?= ${HOST_ARCH}
Configuration ?= debug

# Default V8 directories
LIBV8_INCLUDE_DIR ?= $(shell pwd -L)/third_party/prebuilt-v8/include
LIBV8_LIB_DIR ?= $(shell pwd -L)/third_party/prebuilt-v8/${TARGET_OS}-${ABP_TARGET_ARCH}-${Configuration}

BUILD_DIR ?=$(shell pwd -L)/build

ifeq (${TARGET_OS},android)
Configuration ?= release
ANDROID_PLATFORM_LEVEL=android-16
ANDROID_FIXES=
ifeq ($(ABP_TARGET_ARCH),arm)
ANDROID_ABI = armeabi-v7a
ANDROID_FIXES="LOCAL_LDFLAGS=\"-Wl,--allow-multiple-definition\""
else ifeq ($(ABP_TARGET_ARCH),ia32)
ANDROID_ABI = x86
else ifeq ($(ABP_TARGET_ARCH),arm64)
ANDROID_ABI = arm64-v8a
# minimal platform having arch-arm64, see ndk/platforms
ANDROID_PLATFORM_LEVEL=android-21
else ifeq ($(ABP_TARGET_ARCH),x64)
ANDROID_ABI = x86_64
ANDROID_PLATFORM_LEVEL=android-21
else
$(error "Unsupported Android architecture: $(ABP_TARGET_ARCH)")
endif
endif

# linux,osx
#   x64, ia32
#     debug, release
# android
#   arm, arm64, ia32, x64
#     release

GYP_PARAMETERS=host_arch=${HOST_ARCH} OS=${TARGET_OS} target_arch=${ABP_TARGET_ARCH}
ifneq "$(and ${LIBV8_LIB_DIR}, ${LIBV8_INCLUDE_DIR})" ""
GYP_PARAMETERS+= libv8_lib_dir=${LIBV8_LIB_DIR} libv8_include_dir=${LIBV8_INCLUDE_DIR} libv8_show_warnings=${libv8_show_warnings}
else
$(error "V8 directories are not specified")
endif

TEST_EXECUTABLE = ${BUILD_DIR}/out/Debug/tests

ifdef TEST_RESULTS_XML
TEST_EXECUTABLE += --gtest_output="xml:${TEST_RESULTS_XML}"
endif

# 8.7.220.29
V8_COMMIT = 45d51f3f
URL_PREFIX ?= "https://v8.eyeofiles.com/v8-${V8_COMMIT}"

ifeq ($(WGET_QUIET),true)
WGET_FLAGS=-q
endif

.PHONY: all test clean docs

.DEFAULT_GOAL:=all



test: all
ifdef FILTER
	$(TEST_EXECUTABLE) --gtest_filter=$(FILTER)
else
	$(TEST_EXECUTABLE)
endif

docs:
	doxygen

get-prebuilt-v8:
	$(RM) -r third_party/prebuilt-v8/ third_party/v8-include.tar.xz third_party/v8-prebuilt.tar.xz
	URL_PREFIX=${URL_PREFIX} \
	TARGET_OS=${TARGET_OS} ABP_TARGET_ARCH=${ABP_TARGET_ARCH} \
	TRAVIS_OS_NAME=${TRAVIS_OS_NAME} Configuration=${Configuration} \
	WGET_FLAGS=${WGET_FLAGS} \
	bash build-scripts/prepare-prebuilt-v8.sh

get-v8-commit:
	@echo $(V8_COMMIT)

clean:
	$(RM) -r ${BUILD_DIR} docs

ifeq ($(TARGET_OS),android)
GYP_FILE ?= tests.gyp
all:
	GYP_DEFINES="${GYP_PARAMETERS}" \
	python ./make_gyp_wrapper.py --depth=. -f make-android -Ilibadblockplus.gypi --generator-output=${BUILD_DIR} -Gandroid_ndk_version=r20b ${GYP_FILE}
	$(ANDROID_NDK_ROOT)/ndk-build -C ${BUILD_DIR} installed_modules \
	MAKEFLAGS="${ADDITIONALMAKEFLAGS}" \
	BUILDTYPE=Release \
	APP_ABI=$(ANDROID_ABI) \
	APP_PLATFORM=${ANDROID_PLATFORM_LEVEL} \
	APP_PIE=true \
	APP_STL=c++_shared \
	APP_BUILD_SCRIPT=Makefile \
	NDK_PROJECT_PATH=. \
	NDK_OUT=. \
	${ANDROID_FIXES} \
	NDK_APP_DST_DIR=android-$(ABP_TARGET_ARCH).release
else
SUB_ACTION ?= all
ifdef CXX
CXX_PARAM:=CXX=${CXX}
endif
all:
	GYP_DEFINES="${GYP_PARAMETERS}" third_party/gyp/gyp --depth=. -f make -I libadblockplus.gypi --generator-output=${BUILD_DIR} abpshell.gyp tests.gyp
	$(MAKE) -C ${BUILD_DIR} ${SUB_ACTION} ${CXX_PARAM}

endif
