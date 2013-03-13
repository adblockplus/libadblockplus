{
  'variables': {
    'visibility%': 'hidden',
    'library%': 'static_library',
    'component%': '',
    'want_separate_host_toolset': 0,
  },

  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'cflags': [ '-g', '-O0' ],
      },
      'Release': {
        # Xcode insists on this empty entry.
      },
    },
  },

  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
       or OS=="netbsd"', {
      'target_defaults': {
        'cflags': [ '-Wall', '-W', '-Wno-unused-parameter',
                    '-Wnon-virtual-dtor', '-pthread', '-fno-rtti',
                    '-pedantic' ],
        'ldflags': [ '-pthread', ],
      },
    }],
  ]
}
