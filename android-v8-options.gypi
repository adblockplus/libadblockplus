{
  'variables': {
    'want_separate_host_toolset%': 1,
    'android_ndk_root': '<!(echo ${ANDROID_NDK_ROOT})',
    'clang_dir': '/usr/',
    'make_clang_dir': '/usr/',
  },
  'conditions': [[
    'v8_target_arch=="arm"', {
      'variables': {
        'arm_version%': 7,
        'arm_fpu%': 'default',
        'arm_float_abi%': 'default',
        'arm_thumb%': 'default',
      }
    }
  ]],
  'includes': ['v8.gypi'],
}
