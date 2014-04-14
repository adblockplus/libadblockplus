{
  'variables': {
    'visibility%': 'hidden',
    'library%': 'static_library',
    'component%': '',
    'want_separate_host_toolset': 0,
  },

  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
       or OS=="netbsd" or OS=="android"', {
      'target_defaults': {
        'cflags': [ '-Wall', '-W', '-Wno-unused-parameter',
                    '-Wnon-virtual-dtor', '-pthread', '-fno-rtti',
                    '-pedantic', '-std=c++0x', '-fexceptions', ],
        'ldflags': [ '-pthread', ],
      },
    }],
    ['OS=="mac"', {
      'xcode_settings': {
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        'CLANG_CXX_LIBRARY': 'libstdc++',
        'OTHER_CPLUSPLUSFLAGS' : ['-std=c++0x', '-stdlib=libstdc++'],
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
  }
}
