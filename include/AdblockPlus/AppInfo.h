#ifndef ADBLOCK_PLUS_APP_INFO_H
#define ADBLOCK_PLUS_APP_INFO_H

#include <string>

namespace AdblockPlus
{
  struct AppInfo
  {
    std::string id;
    std::string version;
    std::string name;
    std::string platform;
  };
}

#endif
