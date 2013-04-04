{
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'defines': ['FILTER_ENGINE_STUBS=1'],
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'src/Main.cpp',
      'src/Command.cpp',
      'src/GcCommand.cpp',
      'src/HelpCommand.cpp',
      'src/MatchesCommand.cpp',
      'src/SubscriptionsCommand.cpp'
    ]
  }]
}
