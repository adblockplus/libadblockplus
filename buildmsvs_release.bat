mkdir MSVS\shell
mkdir build\Release\obj\global_intermediate
cd "third_party\v8\tools"
python "js2c.py" "..\..\..\build\Release\obj\global_intermediate\libraries.cc" "CORE" "off" "../src/runtime.js" "../src/v8natives.js" "../src/array.js" "../src/string.js" "../src/uri.js" "../src/math.js" "../src/messages.js" "../src/apinatives.js" "../src/debug-debugger.js" "../src/mirror-debugger.js" "../src/liveedit-debugger.js" "../src/date.js" "../src/json.js" "../src/regexp.js" "../src/macros.py"
python "js2c.py" "..\..\..\build\Release\obj\global_intermediate\experimental-libraries.cc" "EXPERIMENTAL" "off" "../src/runtime.js" "../src/v8natives.js" "../src/array.js" "../src/string.js" "../src/uri.js" "../src/math.js" "../src/messages.js" "../src/apinatives.js" "../src/debug-debugger.js" "../src/mirror-debugger.js" "../src/liveedit-debugger.js" "../src/date.js" "../src/json.js" "../src/regexp.js" "../src/macros.py"
cd ..\..\..
python convert_js.py include\jsSources.cpp
third_party\gyp\gyp --depth=. -f msvs -G msvs_version=2010 --generator-output=MSVS -Dtarget_arch=ia32 -Dcomponent= -Dlibrary=static_library
