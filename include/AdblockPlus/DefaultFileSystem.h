#ifndef ADBLOCK_PLUS_DEFAULT_FILE_SYSTEM_H
#define ADBLOCK_PLUS_DEFAULT_FILE_SYSTEM_H

#include "FileSystem.h"

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

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
    std::string Resolve(const std::string& path) const;
    void SetBasePath(const std::string& path);
  protected:
    std::string basePath;


  };
}

#endif
