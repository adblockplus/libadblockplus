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
    'msvs_configuration_attributes': {
      'OutputDirectory': '<(DEPTH)\\build\\$(ConfigurationName)',
      'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
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
      'googletest/src/gtest-death-test.cc',
      'googletest/src/gtest-filepath.cc',
      'googletest/src/gtest-port.cc',
      'googletest/src/gtest-printers.cc',
      'googletest/src/gtest-test-part.cc',
      'googletest/src/gtest-typed-test.cc',
      'googletest/src/gtest.cc'
    ],
    'include_dirs': [
      'googletest',
      'googletest/include'
    ],
    'direct_dependent_settings': {
      'include_dirs': [
        'googletest',
        'googletest/include'
      ]
    },
    'conditions': [[
      'OS=="win"',
      {
        'defines': [ '_VARIADIC_MAX=10' ],
        'direct_dependent_settings': {
          'defines': [ '_VARIADIC_MAX=10' ],
        },
      }]],
  },
  {
    'target_name': 'googletest_main',
    'type': '<(library)',
    'sources': ['googletest/src/gtest_main.cc'],
    'dependencies': ['googletest'],
    'export_dependent_settings': ['googletest']
  }]
}
