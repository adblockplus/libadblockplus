{
  'variables': {
    'have_curl': '<!(python check_curl.py)'
  },
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
    'all_dependent_settings': {
      'defines': ['FILTER_ENGINE_STUBS=1']
    },
    'dependencies': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'sources': [
      'src/AppInfoJsObject.cpp',
      'src/ConsoleJsObject.cpp',
      'src/DefaultFileSystem.cpp',
      'src/ErrorCallback.cpp',
      'src/FileSystemJsObject.cpp',
      'src/FilterEngine.cpp',
      'src/GlobalJsObject.cpp',
      'src/JsEngine.cpp',
      'src/JsValue.cpp',
      'src/Thread.cpp',
      'src/Utils.cpp',
      'src/WebRequestJsObject.cpp',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cc'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include']
    },
    'export_dependent_settings': ['third_party/v8/tools/gyp/v8.gyp:v8'],
    'conditions': [
      ['have_curl==1',
        {
          'sources': [
            'src/DefaultWebRequestCurl.cpp',
          ],
          'all_dependent_settings': {
            'defines': ['HAVE_CURL'],
            'libraries': ['-lcurl']
          }
        }
      ],
      ['have_curl!=1',
        {
          'sources': [
            'src/DefaultWebRequestDummy.cpp',
          ]
        }
      ]
    ],
    'actions': [{
      'action_name': 'convert_js',
      'variables': {
        'library_files': [
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
        'load_before_files': [
          'lib/compat.js'
        ],
        'load_after_files': [
          'lib/api.js'
        ],
      },
      'inputs': [
        'convert_js.py',
        '<@(library_files)',
        '<@(load_before_files)',
        '<@(load_after_files)',
      ],
      'outputs': [
        '<(INTERMEDIATE_DIR)/adblockplus.js.cpp'
      ],
      'action': [
        'python',
        'convert_js.py',
        'before=<@(load_before_files)',
        '<@(library_files)',
        'after=<@(load_after_files)',
        '<@(_outputs)',
      ]
    }]
  },
  {
    'target_name': 'tests',
    'type': 'executable',
    'dependencies': [
      'third_party/googletest.gyp:googletest_main',
      'libadblockplus'
    ],
    'sources': [
      'test/AppInfoJsObject.cpp',
      'test/ConsoleJsObject.cpp',
      'test/DefaultFileSystem.cpp',
      'test/FileSystemJsObject.cpp',
      'test/FilterEngineStubs.cpp',
      'test/GlobalJsObject.cpp',
      'test/JsEngine.cpp',
      'test/JsValue.cpp',
      'test/Thread.cpp',
      'test/WebRequest.cpp'
    ]
  }]
}
