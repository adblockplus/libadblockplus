{
  'variables': {
    'libv8_include_dir%': 'third_party/v8/include',
    'libv8_build_targets': [],
    'libv8_no_build%': 0,
    'conditions': [[
      'OS=="win"', {
        'libv8_lib_dir%': 'v8/third_party/v8/src/<(CONFIGURATION_NAME)',
        'libv8_libs': [
          '-lv8_libplatform',
          '-lv8_base_0',
          '-lv8_base_1',
          '-lv8_base_2',
          '-lv8_base_3',
          '-lv8_libbase',
          '-lv8_libsampler',
          '-lv8_snapshot',
        ],
      }],
      ['OS=="linux" or OS=="mac"', {
        'libv8_lib_dir%': 'v8/out/<(CONFIGURATION_NAME)',
        'libv8_libs': [
          '-lv8_libplatform',
          '-lv8_base',
          '-lv8_snapshot',
          '-lv8_libbase',
          '-lv8_libsampler',
        ]
      }],
      ['OS=="android"', {
        'libv8_lib_dir%': '', # must be specified, e.g. in Makefile
        'libv8_libs': [
          '<(libv8_lib_dir)/libv8_libplatform.a',
          '<(libv8_lib_dir)/libv8_base.a',
          '<(libv8_lib_dir)/libv8_snapshot.a',
          '<(libv8_lib_dir)/libv8_libbase.a',
          '<(libv8_lib_dir)/libv8_libsampler.a',
        ],
      }],
    ]
  },
  'conditions': [[
    'OS=="win" and libv8_no_build!="true"', {
      'variables': {
        'libv8_build_targets': ['build-v8'],
      },
      'targets': [{
        'target_name': 'build-v8',
        'type': 'none',
        'actions': [{
          'action_name': 'build-v8',
          'inputs': ['build-v8.cmd'],
          'outputs': [
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_libplatform.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_base_0.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_base_1.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_base_2.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_base_3.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_libbase.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_libsampler.lib',
            'build/<(target_arch)/v8/build/<(CONFIGURATION_NAME)/v8_snapshot.lib',
          ],
          'action': [
            'cmd',
            '/C',
            'build-v8.cmd',
            '$(MSBuildBinPath)',
            '<(target_arch)',
            '<(CONFIGURATION_NAME)',
            '$(PlatformToolset)'
          ]
        }],
      }]
    }
  ]],
}
