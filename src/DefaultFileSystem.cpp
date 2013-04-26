#include <AdblockPlus/DefaultFileSystem.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#include <Shlobj.h>
#endif

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif
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

/*
 * In order to get millisecond resolution for modification times, it's necessary to use the 'stat' structure defined in
 * POSIX 2008, which has 'struct timespec st_mtim' instead of 'time_t st_mtime'. Use "#define _POSIX_C_SOURCE 200809L" 
 * before the headers to invoke. The trouble is that not all systems may have this available, a category that includes 
 * MS Windows, and so we'll need multiple implementations.
 */
FileSystem::StatResult DefaultFileSystem::Stat(const std::string& path) const
{
#ifdef WIN32
  struct _stat nativeStat;
  const int failure = _stat(path.c_str(), &nativeStat);
#else
  struct stat nativeStat;
  const int failure = stat(path.c_str(), &nativeStat);
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
  result.lastModified = static_cast<int64_t>(nativeStat.st_mtime) * 1000;
  return result;
}

std::string DefaultFileSystem::Resolve(const std::string& path) const
{
#ifdef WIN32
  // Resolve to LocalLow folder

  OSVERSIONINFOEX osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  BOOL res = GetVersionEx((OSVERSIONINFO*) &osvi);

  if(res == 0 ) return std::string(path);

  std::wstring resolvedW = L"";
  wchar_t resolvedPath[MAX_PATH];
  HRESULT hr;
  if (osvi.dwMajorVersion >= 6)
  {
    hr = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, 0, 0, resolvedPath);
  }
  else
  {
    hr = SHGetFolderPath(NULL, CSIDL_APPDATA, 0, 0, resolvedPath);
  }
  if (FAILED(hr))
    return std::string(path);
  resolvedW.assign(resolvedPath);

  // TODO: Better conversion here
  std::string resolved(resolvedW.begin(), resolvedW.end());
  if (osvi.dwMajorVersion >= 6)
  {
    resolved.append("Low\\AdblockPlus\\");
  }
  else
  {
    resolved.append("\\AdblockPlus\\");
  }
  resolved.append(path);
  return resolved;
#else
  return std::string(path);
#endif
}

