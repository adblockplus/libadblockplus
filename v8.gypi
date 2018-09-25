{
  'variables': {
    'v8_dir%': 'build/v8/',
    'libv8_show_warnings%': 0,
    'libv8_include_dir%': '>(v8_dir)/include',
    'libv8_lib_dir%': '>(v8_dir)/<(OS)-<(target_arch).<(CONFIGURATION_NAME)',
    'conditions': [[
      'OS=="android"', {
        'libv8_libs%': [
          '>(libv8_lib_dir)/libv8_monolith.a',
        ],
      }, {
        'libv8_libs%': [
          '-lv8_monolith',
        ],
      }]
    ]
  },
  'target_defaults': {
    'conditions': [[
      'libv8_show_warnings=="true"', {
        'defines': [
          'V8_DEPRECATION_WARNINGS',
          'V8_IMMINENT_DEPRECATION_WARNINGS'
        ]
      }
    ]]
  }
}
