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
      'src/HelpCommand.cpp'
    ]
  }]
}
