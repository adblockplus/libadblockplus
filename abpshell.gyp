{
  'conditions': [[
    # We don't want to use curl on Windows and Android, skip the check there
    'OS=="win" or OS=="android"',
    {
      'variables': {
        'have_curl': 0
      }
    },
    {
      'variables': {
        'have_curl': '<!(python check_curl.py)'
      }
    }
  ]],
  'targets': [{
    'target_name': 'abpshell',
    'type': 'executable',
    'dependencies': [
      'libadblockplus.gyp:libadblockplus'
    ],
    'sources': [
      'shell/src/Command.cpp',
      'shell/src/Command.h',
      'shell/src/FiltersCommand.cpp',
      'shell/src/FiltersCommand.h',
      'shell/src/GcCommand.cpp',
      'shell/src/GcCommand.h',
      'shell/src/HelpCommand.cpp',
      'shell/src/HelpCommand.h',
      'shell/src/Main.cpp',
      'shell/src/MatchesCommand.cpp',
      'shell/src/MatchesCommand.h',
      'shell/src/PrefsCommand.cpp',
      'shell/src/PrefsCommand.h',
      'shell/src/SubscriptionsCommand.cpp',
      'shell/src/SubscriptionsCommand.h',
      'shell/src/WebRequestCurl.cpp',
      'shell/src/WebRequestCurl.h',
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
