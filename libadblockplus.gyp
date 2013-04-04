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
    'defines': ['FILTER_ENGINE_STUBS=1'],
    'dependencies': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'sources': [
      'src/ConsoleJsObject.cpp',
      'src/ErrorCallback.cpp',
      'src/FileReader.cpp',
      'src/FilterEngine.cpp',
      'src/GlobalJsObject.cpp',
      'src/JsEngine.cpp',
      'src/Thread.cpp',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cc'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include']
    },
    'export_dependent_settings': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'actions': [{
      'action_name': 'convert_js',
      'variables': {
        'core_library_files': [
          'lib/info.js',
          'lib/io.js',
          'lib/prefs.js',
          'lib/utils.js',
          'lib/elemHideHitRegistration.js',
          'adblockplus/lib/filterNotifier.js',
          'adblockplus/lib/filterClasses.js',
          'adblockplus/lib/subscriptionClasses.js',
          'adblockplus/lib/filterStorage.js',
          'adblockplus/lib/elemHide.js',
          'adblockplus/lib/matcher.js',
          'adblockplus/lib/filterListener.js',
          'adblockplus/lib/synchronizer.js',
        ],
        'additional_library_files': [
          'lib/compat.js'
        ],
      },
      'inputs': [
        'convert_js.py',
        '<@(core_library_files)',
        '<@(additional_library_files)',
      ],
      'outputs': [
        '<(INTERMEDIATE_DIR)/adblockplus.js.cpp'
      ],
      'action': [
        'python',
        'convert_js.py',
        '<@(core_library_files)',
        '--',
        '<@(additional_library_files)',
        '<@(_outputs)',
      ]
    }]
  },
  {
    'target_name': 'tests',
    'type': 'executable',
    'defines': ['FILTER_ENGINE_STUBS=1'],
    'dependencies': [
      'third_party/googletest.gyp:googletest_main',
      'libadblockplus'
    ],
    'sources': [
      'test/ConsoleJsObject.cpp',
      'test/FilterEngineStubs.cpp',
      'test/JsEngine.cpp',
      'test/Thread.cpp'
    ]
  }]
}
