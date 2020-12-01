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
          '_ITERATOR_DEBUG_LEVEL=0' #Disabled due to linking errors on Windows 64 build, see: https://eyeogmbh.atlassian.net/browse/DP-12
        ],
        'link_settings': {
          'libraries': ['-lDbgHelp'],
        },
      }
    }],
    ['OS=="mac"', {
      'xcode_settings': {
        'ARCHS': ["x86_64"]
      }
    }],
    ['target_arch=="x64" or target_arch=="arm64"', {
      'target_defaults': {
        # https://www.mail-archive.com/v8-dev@googlegroups.com/msg160557.html
        'cflags_cc': [ '-DV8_COMPRESS_POINTERS', '-DV8_31BIT_SMIS_ON_64BIT_ARCH' ]
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
