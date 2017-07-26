{
  'conditions': [[
    # We don't want to use curl on Windows and Android, skip the check there
    'OS=="win" or OS=="android"',
    {
      'variables': {
        'have_curl': 0
      }
    },
    {
      'variables': {
        'have_curl': '<!(python check_curl.py)'
      }
    }
  ]],
  'includes': ['v8.gypi', 'shell/shell.gyp'],
  'targets': [{
    'target_name': 'libadblockplus',
    'type': '<(library)',
    'dependencies': ['<@(libv8_build_targets)'],
    'xcode_settings':{},
    'include_dirs': [
      'include',
      '<(libv8_include_dir)'
    ],
    'sources': [
      'include/AdblockPlus/ITimer.h',
      'include/AdblockPlus/IWebRequest.h',
      'include/AdblockPlus/IFileSystem.h',
      'include/AdblockPlus/DefaultFileSystem.h',
      'src/AppInfoJsObject.cpp',
      'src/ConsoleJsObject.cpp',
      'src/DefaultLogSystem.cpp',
      'src/DefaultFileSystem.cpp',
      'src/DefaultTimer.cpp',
      'src/DefaultTimer.h',
      'src/DefaultWebRequest.h',
      'src/DefaultWebRequest.cpp',
      'src/FileSystemJsObject.cpp',
      'src/FilterEngine.cpp',
      'src/GlobalJsObject.cpp',
      'src/JsContext.cpp',
      'src/JsEngine.cpp',
      'src/JsError.cpp',
      'src/JsValue.cpp',
      'src/Notification.cpp',
      'src/ReferrerMapping.cpp',
      'src/Thread.cpp',
      'src/Utils.cpp',
      'src/WebRequestJsObject.cpp',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cpp'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include'],
      'conditions': [[
        'OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalLibraryDirectories': ['<(libv8_lib_dir)'],
            }
          }
        }
      ]],
    },
    'conditions': [
      ['OS=="linux" or OS=="mac"', {
        'link_settings': {
          'libraries': [
            '<@(libv8_libs)'
          ],
          'library_dirs': [
            '<(libv8_lib_dir)'
          ]
        }
      }],
      ['OS=="win"', {
        'link_settings': {
          'libraries': [
            '<@(libv8_libs)',
            '-lwinmm'
          ],
        },
      }],
      ['OS=="android"', {
        'user_libraries': [
          '<@(libv8_libs)'
        ],
        'standalone_static_library': 1, # disable thin archives
      }],
      ['have_curl==1',
        {
          'sources': [
            'src/DefaultWebRequestCurl.cpp',
          ],
          'link_settings': {
            'libraries': ['-lcurl']
          },
          'all_dependent_settings': {
            'defines': ['HAVE_CURL'],
          }
        }
      ],
      ['OS=="win"',
        {
          'sources': [
            'src/DefaultWebRequestWinInet.cpp',
          ],
          'link_settings': {
            'libraries': [ '-lshlwapi.lib', '-lwinhttp.lib' ]
          }
        }
      ],
      ['have_curl!=1 and OS!="win"',
        {
          'sources': [
            'src/DefaultWebRequestDummy.cpp',
          ]
        }
      ],
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
          'adblockpluscore/lib/events.js',
          'adblockpluscore/lib/coreUtils.js',
          'adblockpluscore/lib/filterNotifier.js',
          'lib/init.js',
          'adblockpluscore/lib/filterClasses.js',
          'adblockpluscore/lib/subscriptionClasses.js',
          'adblockpluscore/lib/filterStorage.js',
          'adblockpluscore/lib/elemHide.js',
          'adblockpluscore/lib/cssRules.js',
          'adblockpluscore/lib/matcher.js',
          'adblockpluscore/lib/filterListener.js',
          'adblockpluscore/lib/downloader.js',
          'adblockpluscore/lib/notification.js',
          'lib/notificationShowRegistration.js',
          'adblockpluscore/lib/synchronizer.js',
          'lib/filterUpdateRegistration.js',
          'adblockpluscore/chrome/content/ui/subscriptions.xml',
          'lib/updater.js',
        ],
        'load_before_files': [
          'lib/compat.js'
        ],
        'load_after_files': [
          'lib/api.js',
          'lib/publicSuffixList.js',
          'lib/punycode.js',
          'lib/basedomain.js',
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
        '<@(_outputs)',
        '--before', '<@(load_before_files)',
        '--convert', '<@(library_files)',
        '--after', '<@(load_after_files)',
      ]
    }]
  },
  {
    'target_name': 'tests',
    'type': 'executable',
    'xcode_settings': {},
    'dependencies': [
      'googletest.gyp:googletest_main',
      'libadblockplus'
    ],
    'sources': [
      'test/BaseJsTest.h',
      'test/BaseJsTest.cpp',
      'test/AppInfoJsObject.cpp',
      'test/ConsoleJsObject.cpp',
      'test/DefaultFileSystem.cpp',
      'test/FileSystemJsObject.cpp',
      'test/FilterEngine.cpp',
      'test/GlobalJsObject.cpp',
      'test/JsEngine.cpp',
      'test/JsValue.cpp',
      'test/Notification.cpp',
      'test/Prefs.cpp',
      'test/ReferrerMapping.cpp',
      'test/Thread.cpp',
      'test/UpdateCheck.cpp',
      'test/WebRequest.cpp'
    ],
    'msvs_settings': {
      'VCLinkerTool': {
        'SubSystem': '1',   # Console
        'EntryPointSymbol': 'mainCRTStartup',
      },
    },
  }]
}
