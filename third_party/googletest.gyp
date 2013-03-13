{
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
    'direct_dependent_settings': {
      'include_dirs': [
        'googletest',
        'googletest/include'
      ]
    }
  },
  {
    'target_name': 'googletest_main',
    'type': '<(library)',
    'sources': ['googletest/src/gtest_main.cc'],
    'dependencies': ['googletest'],
    'export_dependent_settings': ['googletest']
  }]
}
