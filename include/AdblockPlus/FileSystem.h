#ifndef ADBLOCK_PLUS_FILE_SYSTEM_H
#define ADBLOCK_PLUS_FILE_SYSTEM_H

#include <istream>
#include <stdint.h>
#include <string>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class FileSystem
  {
  public:
    struct StatResult
    {
      bool exists;
      bool isDirectory;
      bool isFile;
      int64_t lastModified;
    };

    virtual ~FileSystem() {}
    virtual std::tr1::shared_ptr<std::istream>
      Read(const std::string& path) const = 0;
    virtual void Write(const std::string& path,
                       std::tr1::shared_ptr<std::ostream> data) = 0;
    virtual void Move(const std::string& fromPath,
                      const std::string& toPath) = 0;
    virtual void Remove(const std::string& path) = 0;
    virtual StatResult Stat(const std::string& path) const = 0;
  };

  typedef std::tr1::shared_ptr<FileSystem> FileSystemPtr;
}

#endif
