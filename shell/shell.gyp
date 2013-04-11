{
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'src/Main.cpp',
      'src/Command.cpp',
      'src/GcCommand.cpp',
      'src/HelpCommand.cpp',
      'src/MatchesCommand.cpp',
      'src/FiltersCommand.cpp',
      'src/SubscriptionsCommand.cpp'
    ]
  }]
}
