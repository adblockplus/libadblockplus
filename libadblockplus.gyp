{
  'includes': ['third_party/v8/build/common.gypi'],
  'targets': [{
    'target_name': 'libadblockplus',
    'type': '<(library)',
    'include_dirs': ['third_party/v8/include'],
    'dependencies': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'sources': [
      'src/FileReader.cpp',
      'src/JsEngine.cpp'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['src']
    },
    'export_dependent_settings': ['third_party/v8/tools/gyp/v8.gyp:v8']
  },
  {
    'target_name': 'tests',
    'type': 'executable',
    'dependencies': [
      'third_party/gtest/googletest.gyp:gtest_main',
      'libadblockplus'
    ],
    'sources': [
      'test/JsEngine.cpp'
    ]
  }]
}
