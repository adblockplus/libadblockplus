{
  'includes': ['common.gypi'],
  'variables': {
    'v8_enable_i18n_support': 0,
    'v8_optimized_debug': 0,
    'v8_use_external_startup_data': 0,
    'v8_use_snapshot': 1,
    'v8_random_seed%': 0,
    'v8_android_log_stdout': 1,
  },

  'target_defaults': {
    'defines': [
      '__ANDROID_API__=16',
    ],
  },
  'conditions': [
    ['OS=="linux" or OS=="mac"', {
      'target_defaults': {
        'cflags': [ '-fPIC' ],
        'cflags_cc': [ '-fPIC' ]
      }
    }],
    ['OS=="win"', {
      'target_defaults': {
        'configurations': {
          'Debug': {
            'conditions': [
              ['target_arch=="x64"', {
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'AdditionalOptions': [
                      '/bigobj', # compiling fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
                    ]
                  }
                }
              }]
            ],
          }
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WarningLevel': 0,
          },
        },
      }
    }]
  ],
}
