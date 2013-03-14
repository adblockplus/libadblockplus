{
  'variables': {
    'visibility%': 'hidden',
    'library%': 'static_library',
    'component%': '',
    'want_separate_host_toolset': 0,
  },

  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
       or OS=="netbsd"', {
      'target_defaults': {
        'cflags': [ '-Wall', '-W', '-Wno-unused-parameter',
                    '-Wnon-virtual-dtor', '-pthread', '-fno-rtti',
                    '-pedantic',
                    # Ignore some warnings for googletest
                    '-Wno-error=long-long', '-Wno-error=variadic-macros',
                    '-Wno-error=missing-field-initializers' ],
        'ldflags': [ '-pthread', ],
      },
    }],
  ]
}
