#ifndef ADBLOCK_PLUS_DEFAULT_FILE_SYSTEM_H
#define ADBLOCK_PLUS_DEFAULT_FILE_SYSTEM_H

#include "FileSystem.h"

namespace AdblockPlus
{
  class DefaultFileSystem : public FileSystem
  {
  public:
    std::tr1::shared_ptr<std::istream> Read(const std::string& path) const;
    void Write(const std::string& path,
               std::tr1::shared_ptr<std::ostream> data);
    void Move(const std::string& fromPath,
                      const std::string& toPath);
    void Remove(const std::string& path);
    StatResult Stat(const std::string& path) const;
  };
}

#endif
