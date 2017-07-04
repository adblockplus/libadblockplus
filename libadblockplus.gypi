{
  'includes': ['common.gypi'],
  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
       or OS=="netbsd" or OS=="android"', {
      'target_defaults': {
        'cflags_cc': [ '-Wall', '-W', '-Wno-unused-parameter',
                    '-Wnon-virtual-dtor', '-pedantic' ]
      }
    }],
    ['OS=="linux" and target_arch=="ia32"', {
      'target_defaults': {
        'cflags_cc': [ '-m32' ]
      }
    }],
    ['OS=="win"', {
      'target_defaults': {
        'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64'
          }]
        ],
        'msvs_configuration_attributes': {
          'CharacterSet': '1',
        },
        'defines': [
          'WIN32',
        ],
        'link_settings': {
          'libraries': ['-lDbgHelp'],
        },
      }
    }],
    ['OS=="mac" and target_arch=="ia32"', {
      'xcode_settings': {
        'ARCHS': ["i386"]
      }
    }],
  ],

  'target_defaults': {
    'configurations': {
      'Debug': {
        'defines': [
          'DEBUG'
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'conditions': [
              ['component=="shared_library"', {
                'RuntimeLibrary': '3',  #/MDd
              }, {
                'RuntimeLibrary': '1',  #/MTd
              }
            ]]
          }
        }
      },
      'Release': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'conditions': [
              ['component=="shared_library"', {
                'RuntimeLibrary': '2',  #/MD
              }, {
                'RuntimeLibrary': '0',  #/MT
              }
            ]]
          }
        }
      }
    },
  }
}
