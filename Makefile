.PHONY: test

all:
	third_party/gyp/gyp --depth=. -f make --generator-output=build -Dtarget_arch=x64 -Dcomponent= -Dlibrary=static_library
	$(MAKE) -C build

test: all
	build/out/Debug/tests

clean:
	$(RM) -r build
