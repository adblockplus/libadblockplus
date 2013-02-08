{
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'src/Main.cpp'
    ]
  }]
}
