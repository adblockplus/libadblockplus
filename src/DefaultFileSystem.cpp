#include <AdblockPlus/DefaultFileSystem.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

#ifndef WIN32
#include <cerrno>
#include <sys/stat.h>
#endif

#include "../src/Utils.h"

using namespace AdblockPlus;

namespace
{
  class RuntimeErrorWithErrno : public std::runtime_error
  {
  public:
    explicit RuntimeErrorWithErrno(const std::string& message)
      : std::runtime_error(message + " (" + strerror(errno) + ")")
    {
    }
  };
}

std::tr1::shared_ptr<std::istream>
DefaultFileSystem::Read(const std::string& path) const
{
  return std::tr1::shared_ptr<std::istream>(new std::ifstream(path.c_str()));
}

void DefaultFileSystem::Write(const std::string& path,
                              std::tr1::shared_ptr<std::ostream> data)
{
  std::ofstream file(path.c_str());
  file << Utils::Slurp(*data);
}

void DefaultFileSystem::Move(const std::string& fromPath,
                             const std::string& toPath)
{
  if (rename(fromPath.c_str(), toPath.c_str()))
    throw RuntimeErrorWithErrno("Failed to move " + fromPath + " to " + toPath);
}

void DefaultFileSystem::Remove(const std::string& path)
{
  if (remove(path.c_str()))
    throw RuntimeErrorWithErrno("Failed to remove " + path);
}

FileSystem::StatResult DefaultFileSystem::Stat(const std::string& path) const
{
#ifdef WIN32
  struct _stat64 nativeStat;
  const int failure = _stat64(path.c_str(), &nativeStat);
#else
  struct stat64 nativeStat;
  const int failure = stat64(path.c_str(), &nativeStat);
#endif
  if (failure) {
    if (errno == ENOENT)
      return FileSystem::StatResult();
    throw RuntimeErrorWithErrno("Unable to stat " + path);
  }
  FileSystem::StatResult result;
  result.exists = true;
  result.isFile = S_ISREG(nativeStat.st_mode);
  result.isDirectory = S_ISDIR(nativeStat.st_mode);
  result.lastModified = nativeStat.st_mtime;
  return result;
}
