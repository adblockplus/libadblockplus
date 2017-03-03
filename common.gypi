{
  'variables': {
    'visibility%': 'hidden',
    'library%': 'static_library',
    'component%': '',
    'want_separate_host_toolset': 0,
    'v8_optimized_debug': 0,
    'v8_enable_i18n_support': 0,
  },

  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
       or OS=="netbsd" or OS=="android"', {
      'target_defaults': {
        # V8 headers cause a build error on recent gcc.
        # Adding -fpermissive to solve this.
        # See https://issues.adblockplus.org/ticket/4950
        # We might be able to do without after upgrading V8.
        'cflags': [ '-Wall', '-W', '-Wno-unused-parameter',
                    '-Wnon-virtual-dtor', '-pthread', '-fno-rtti',
                    '-pedantic', '-std=c++0x', '-fexceptions', '-fpermissive' ],
        'cflags!': [ '-Werror', ],
        'ldflags': [ '-pthread', ],
      },
    }],
    ['OS=="mac"', {
      'xcode_settings': {
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
        'CLANG_CXX_LIBRARY': 'libc++',
        'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11', '-stdlib=libc++'],
      },
    }],
    ['OS=="android"', {
      'target_defaults': {
        'cflags!': [
          '-pthread',  # Not supported by Android toolchain.
        ],
        'ldflags!': [
          '-pthread',  # Not supported by Android toolchain.
        ],
      },
    }],
  ],

  'target_defaults': {
    'msvs_cygwin_shell': 0,
    'target_conditions': [[
      'OS=="mac" and _type=="executable"', {
        'xcode_settings': {
          'OTHER_LDFLAGS': ['-stdlib=libc++'],
        },
      }
    ]],
  }
}
