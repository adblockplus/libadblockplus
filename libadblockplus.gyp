{
  'includes': ['v8.gypi'],
  'variables': {
    'library_files': [
      'lib/info.js',
      'lib/io.js',
      'lib/prefs.js',
      'lib/utils.js',
      'lib/elemHideHitRegistration.js',
      'adblockpluscore/lib/time.js',
      'adblockpluscore/lib/events.js',
      'adblockpluscore/lib/caching.js',
      'adblockpluscore/data/publicSuffixList.json',
      'adblockpluscore/data/subscriptions.json',
      'adblockpluscore/data/resources.json',
      'adblockpluscore/lib/url.js',
      'adblockpluscore/lib/filterNotifier.js',
      'adblockpluscore/lib/recommendations.js',
      'adblockpluscore/lib/common.js',
      'adblockpluscore/lib/elemHideExceptions.js',
      'adblockpluscore/lib/contentTypes.js',
      'adblockpluscore/lib/filterState.js',
      'adblockpluscore/lib/filterClasses.js',
      'adblockpluscore/lib/snippets.js',
      'adblockpluscore/lib/analytics.js',
      'adblockpluscore/lib/downloader.js',
      'adblockpluscore/lib/subscriptionClasses.js',
      'adblockpluscore/lib/iniParser.js',
      'adblockpluscore/lib/filterStorage.js',
      'adblockpluscore/lib/filtersByDomain.js',
      'adblockpluscore/lib/elemHide.js',
      'adblockpluscore/lib/elemHideEmulation.js',
      'adblockpluscore/lib/patterns.js',
      'adblockpluscore/lib/matcher.js',
      'adblockpluscore/lib/filterListener.js',
      'adblockpluscore/lib/filterEngine.js',
      'adblockpluscore/lib/versions.js',
      'adblockpluscore/lib/synchronizer.js',
      'lib/filterUpdateRegistration.js',
      'lib/compose.js',
      'adblockpluscore/lib/jsbn.js',
      'adblockpluscore/lib/rusha.js',
      'adblockpluscore/lib/rsa.js',
      'lib/init.js',
      'lib/punycode.js',
    ],
    'load_before_files': [
      'lib/compat.js'
    ],
    'load_after_files': [
      'lib/api.js',
      'lib/uri.js',
    ],
    'all_files': [
      '<@(load_before_files)',
      '<@(library_files)',
      '<@(load_after_files)',
    ],
  },
  'targets': [{
    'target_name': 'libadblockplus',
    'type': '<(library)',
    'xcode_settings':{},
    'include_dirs': [
      'include',
      '<(libv8_include_dir)'
    ],
    'sources': [
      'include/AdblockPlus/AppInfo.h',
      'include/AdblockPlus/Filter.h',
      'include/AdblockPlus/FilterEngineFactory.h',
      'include/AdblockPlus/IElement.h',
      'include/AdblockPlus/IExecutor.h',
      'include/AdblockPlus/IFileSystem.h',
      'include/AdblockPlus/IFilterEngine.h',
      'include/AdblockPlus/IFilterImplementation.h',
      'include/AdblockPlus/IResourceReader.h',
      'include/AdblockPlus/ISubscriptionImplementation.h',
      'include/AdblockPlus/ITimer.h',
      'include/AdblockPlus/IV8IsolateProvider.h',
      'include/AdblockPlus/IWebRequest.h',
      'include/AdblockPlus/JSValue.h',
      'include/AdblockPlus/Platform.h',
      'include/AdblockPlus/PlatformFactory.h',
      'include/AdblockPlus/ReferrerMapping.h',
      'include/AdblockPlus/Subscription.h',
      'src/ActiveObject.cpp',
      'src/ActiveObject.h',
      'src/AsyncExecutor.cpp',
      'src/AsyncExecutor.h',
      'src/AppInfoJsObject.cpp',
      'src/AppInfoJsObject.h',
      'src/ConsoleJsObject.cpp',
      'src/ConsoleJsObject.h',
      'src/DefaultFileSystem.cpp',
      'src/DefaultFileSystem.h',
      'src/DefaultFilterEngine.cpp',
      'src/DefaultFilterEngine.h',
      'src/DefaultFilterImplementation.cpp',
      'src/DefaultFilterImplementation.h',
      'src/DefaultLogSystem.cpp',
      'src/DefaultLogSystem.h',
      'src/DefaultPlatform.cpp',
      'src/DefaultPlatform.h',
      'src/DefaultResourceReader.cpp',
      'src/DefaultResourceReader.h',
      'src/DefaultSubscriptionImplementation.cpp',
      'src/DefaultSubscriptionImplementation.h',
      'src/DefaultTimer.cpp',
      'src/DefaultTimer.h',
      'src/DefaultWebRequest.cpp',
      'src/DefaultWebRequest.h',
      'src/FileSystemJsObject.cpp',
      'src/FileSystemJsObject.h',
      'src/Filter.cpp',
      'src/FilterEngineFactory.cpp',
      'src/GlobalJsObject.cpp',
      'src/GlobalJsObject.h',
      'src/ElementUtils.cpp',
      'src/ElementUtils.h',
      'src/IFilterEngine.cpp',
      'src/JsContext.cpp',
      'src/JsContext.h',
      'src/JsEngine.cpp',
      'src/JsEngine.h',
      'src/JsError.cpp',
      'src/JsError.h',
      'src/JsValue.cpp',
      'src/PlatformFactory.cpp',
      'src/ReferrerMapping.cpp',
      'src/ResourceReaderJsObject.cpp',
      'src/ResourceReaderJsObject.h',
      'src/Subscription.cpp',
      'src/SynchronizedCollection.h',
      'src/Thread.cpp',
      'src/Thread.h',
      'src/Utils.cpp',
      'src/Utils.h',
      'src/WebRequestJsObject.cpp',
      'src/WebRequestJsObject.h',
      '<(INTERMEDIATE_DIR)/adblockplus.js.cpp'
    ],
    'defines': [
      'ABP_SCRIPT_FILES="<@(all_files)"',
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
      }]
    ],
    'actions': [{
      'action_name': 'convert_js',
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
