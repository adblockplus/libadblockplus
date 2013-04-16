{
  'variables': {
    'python%': 'python',
    'msvs_cygwin_shell': 0
  },
  'conditions': [
    [
        # Put OS values known not to support curl in the expression below
        'OS=="win"',      
        { 'variables': { 'have_curl': 0 }},
        { 'variables' :{ 'have_curl': '<!(<(python) check_curl.py' }}
    ]
  ],
  'includes': ['third_party/v8/build/common.gypi'],
  'targets': [{
    'target_name': 'libadblockplus_solo',
    'type': '<(library)',
    'msvs_cygwin_shell': 0,
    'include_dirs': [
      'include',
      'third_party/v8/include'
    ],
    'defines': ['FILTER_ENGINE_STUBS=1'],
    'all_dependent_settings': {
      'defines': ['FILTER_ENGINE_STUBS=1']
    },
    'sources': [
      'src/ConsoleJsObject.cpp',
      'src/ErrorCallback.cpp',
      'src/FileReader.cpp',
      'src/FilterEngine.cpp',
      'src/GlobalJsObject.cpp',
      'src/JsEngine.cpp',
      'src/Thread.cpp',
      'src/WebRequestJsObject.cpp',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cc'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include']
    },
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
        '<(python)',
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
    'dependencies': [
      'libadblockplus_solo',
      #'third_party/googletest.gyp:googletest_main',
    ],
    'sources': [
      'test/ConsoleJsObject.cpp',
      'test/FilterEngineStubs.cpp',
      'test/GlobalJsObject.cpp',
      'test/JsEngine.cpp',
      'test/Thread.cpp',
      'test/WebRequest.cpp'
    ]
  }]
}
