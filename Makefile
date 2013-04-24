ARCH := x64

.PHONY: test

all:
	third_party/gyp/gyp --depth=. -f make -I common.gypi --generator-output=build -Dtarget_arch=$(ARCH) libadblockplus.gyp
	$(MAKE) -C build

test: all
	build/out/Debug/tests

clean:
	$(RM) -r build
