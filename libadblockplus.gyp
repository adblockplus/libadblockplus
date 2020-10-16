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
  'includes': ['v8.gypi'],
  'targets': [{
    'target_name': 'libadblockplus',
    'type': '<(library)',
    'xcode_settings':{},
    'include_dirs': [
      'include',
      '<(libv8_include_dir)'
    ],
    'sources': [
      'include/AdblockPlus/ActiveObject.h',
      'include/AdblockPlus/AsyncExecutor.h',
      'include/AdblockPlus/Filter.h',
      'include/AdblockPlus/FilterEngineFactory.h',
      'include/AdblockPlus/IElement.h',
      'include/AdblockPlus/IFileSystem.h',
      'include/AdblockPlus/IFilterEngine.h',
      'include/AdblockPlus/IFilterImplementation.h',
      'include/AdblockPlus/ITimer.h',
      'include/AdblockPlus/IWebRequest.h',
      'include/AdblockPlus/Platform.h',
      'include/AdblockPlus/Scheduler.h',
      'include/AdblockPlus/SynchronizedCollection.h',
      'src/ActiveObject.cpp',
      'src/AsyncExecutor.cpp',
      'src/AppInfoJsObject.cpp',
      'src/ConsoleJsObject.cpp',
      'src/DefaultLogSystem.cpp',
      'src/DefaultFileSystem.h',
      'src/DefaultFileSystem.cpp',
      'src/DefaultFilterImplementation.h',
      'src/DefaultFilterImplementation.cpp',
      'src/DefaultFilterEngine.h',
      'src/DefaultFilterEngine.cpp',
      'src/DefaultSubscriptionImplementation.h',
      'src/DefaultSubscriptionImplementation.cpp',
      'src/DefaultTimer.cpp',
      'src/DefaultTimer.h',
      'src/DefaultWebRequest.h',
      'src/DefaultWebRequest.cpp',
      'src/FileSystemJsObject.cpp',
      'src/Filter.cpp',
      'src/FilterEngineFactory.cpp',
      'src/GlobalJsObject.cpp',
      'src/ElementUtils.h',
      'src/ElementUtils.cpp',
      'src/IFilterEngine.cpp',
      'src/JsContext.cpp',
      'src/JsEngine.cpp',
      'src/JsError.cpp',
      'src/JsValue.cpp',
      'src/Platform.cpp',
      'src/ReferrerMapping.cpp',
      'src/Subscription.cpp',
      'src/Thread.cpp',
      'src/Utils.cpp',
      'src/WebRequestJsObject.cpp',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cpp'
    ],
    'direct_dependent_settings': {
      'include_dirs': ['include'],
      'conditions': [[
        'OS=="win"', {
          'link_settings': {
            'libraries': [
              '<@(libv8_libs)',
              '-lwinmm'
            ],
          },
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalLibraryDirectories': ['<(libv8_lib_dir)'],
            }
          }
        }
      ], ['OS=="linux" or OS=="mac"', {
        'link_settings': {
          'libraries': [
            '<@(libv8_libs)'
          ],
          'library_dirs': [
            '<(libv8_lib_dir)'
          ]
        }
      }], ['OS=="android"', {
        'user_libraries': [
          '<@(libv8_libs)'
        ],
      }]],
    },
    'conditions': [
      ['OS=="android"', {
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
          'lib/compose.js',
          'adblockpluscore/lib/events.js',
          'adblockpluscore/lib/caching.js',
          'adblockpluscore/data/publicSuffixList.json',
          'adblockpluscore/data/subscriptions.json',
          'adblockpluscore/data/resources.json',
          'adblockpluscore/lib/url.js',
          'adblockpluscore/lib/filterNotifier.js',
          'adblockpluscore/lib/recommendations.js',
          'adblockpluscore/lib/common.js',
          'adblockpluscore/lib/time.js',
          'adblockpluscore/lib/contentTypes.js',
          'adblockpluscore/lib/filterClasses.js',
          'adblockpluscore/lib/filterState.js',
          'adblockpluscore/lib/analytics.js',
          'adblockpluscore/lib/downloader.js',
          'adblockpluscore/lib/subscriptionClasses.js',
          'adblockpluscore/lib/iniParser.js',
          'adblockpluscore/lib/filterStorage.js',
          'adblockpluscore/lib/filtersByDomain.js',
          'adblockpluscore/lib/elemHideExceptions.js',
          'adblockpluscore/lib/elemHide.js',
          'adblockpluscore/lib/elemHideEmulation.js',
          'adblockpluscore/lib/matcher.js',
          'adblockpluscore/lib/patterns.js',
          'adblockpluscore/lib/snippets.js',
          'adblockpluscore/lib/filterListener.js',
          'adblockpluscore/lib/filterEngine.js',
          'adblockpluscore/lib/versions.js',
          'adblockpluscore/lib/jsbn.js',
          'adblockpluscore/lib/rusha.js',
          'adblockpluscore/lib/rsa.js',
          'lib/init.js',
          'adblockpluscore/lib/synchronizer.js',
          'lib/filterUpdateRegistration.js',
        ],
        'load_before_files': [
          'lib/compat.js'
        ],
        'load_after_files': [
          'lib/api.js',
          'lib/punycode.js',
          'lib/uri.js',
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
  }]
}
