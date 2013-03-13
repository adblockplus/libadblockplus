ARCH := x64

.PHONY: test

all:
	third_party/gyp/gyp --depth=. -f make -I third_party/v8/build/standalone.gypi --generator-output=build -Dtarget_arch=$(ARCH) -Dcomponent= -Dlibrary=static_library
	$(MAKE) -C build

test: all
	build/out/Debug/tests

clean:
	$(RM) -r build
