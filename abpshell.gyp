{
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'shell/src/Main.cpp',
      'shell/src/Command.cpp',
      'shell/src/GcCommand.cpp',
      'shell/src/HelpCommand.cpp',
      'shell/src/FiltersCommand.cpp',
      'shell/src/MatchesCommand.cpp',
      'shell/src/PrefsCommand.cpp',
      'shell/src/SubscriptionsCommand.cpp'
    ],
    'msvs_settings': {
      'VCLinkerTool': {
        'SubSystem': '1',   # Console
      }
    },
    'xcode_settings': {
      'OTHER_LDFLAGS': ['-stdlib=libstdc++'],
    },
  }]
}
