{
  'target_defaults': {
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': '0',
            'conditions': [
              ['OS=="win" and component=="shared_library"', {
                'RuntimeLibrary': '3',  # /MDd
              }, {
                'RuntimeLibrary': '1',  # /MTd
              }]
            ],
          },
        },
      },
      'Release': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': '2',
            'InlineFunctionExpansion': '2',
            'EnableIntrinsicFunctions': 'true',
            'FavorSizeOrSpeed': '0',
            'StringPooling': 'true',

            'conditions': [
              ['OS=="win" and component=="shared_library"', {
                'RuntimeLibrary': '2',  # /MD
              }, {
                'RuntimeLibrary': '0',  # /MT
              }]
            ],
          },
        },
      },
    },
    'conditions': [[
      'target_arch=="x64"', {
        'xcode_settings': {
          'ARCHS': [ 'x86_64' ],
        },
        'msvs_configuration_platform': 'x64',
      }, {
        'xcode_settings': {
          'ARCHS': [ 'i386' ],
        },
      },
    ]]
  },
  'targets': [{
    'target_name': 'googletest',
    'type': '<(library)',
    'sources': [
      'third_party/googletest/googletest/src/gtest-death-test.cc',
      'third_party/googletest/googletest/src/gtest-filepath.cc',
      'third_party/googletest/googletest/src/gtest-port.cc',
      'third_party/googletest/googletest/src/gtest-printers.cc',
      'third_party/googletest/googletest/src/gtest-test-part.cc',
      'third_party/googletest/googletest/src/gtest-typed-test.cc',
      'third_party/googletest/googletest/src/gtest.cc'
    ],
    'include_dirs': [
      'third_party/googletest/googletest',
      'third_party/googletest/googletest/include'
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        'third_party/googletest/googletest',
        'third_party/googletest/googletest/include'
      ]
    },
  },
  {
    'target_name': 'googletest_main',
    'type': '<(library)',
    'sources': ['third_party/googletest/googletest/src/gtest_main.cc'],
    'dependencies': ['googletest'],
    'export_dependent_settings': ['googletest']
  }]
}
