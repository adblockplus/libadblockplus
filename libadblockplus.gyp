{
  'includes': ['third_party/v8/build/common.gypi',
               'shell/shell.gyp'],
  'targets': [{
    'target_name': 'libadblockplus',
    'type': '<(library)',
    'include_dirs': [
      'include',
      'third_party/v8/include'
    ],
    'dependencies': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'sources': [
      'src/ConsoleJsObject.cpp',
      'src/ErrorCallback.cpp',
      'src/FileReader.cpp',
      'src/FilterEngine.cpp',
      'src/JsEngine.cpp'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include']
    },
    'export_dependent_settings': ['third_party/v8/tools/gyp/v8.gyp:v8']
  },
  {
    'target_name': 'tests',
    'type': 'executable',
    'dependencies': [
      'third_party/googletest.gyp:googletest_main',
      'libadblockplus'
    ],
    'sources': [
      'test/ConsoleJsObject.cpp',
      'test/FilterEngineStubs.cpp',
      'test/JsEngine.cpp'
    ]
  }]
}
