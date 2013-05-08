/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#include <Shlwapi.h>
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
  if (basePath == "")
  {
    return path;
  }
  else
  {
#ifdef _WIN32
  if (PathIsRelative(Utils::ToUTF16String(path, path.length()).c_str()))
#else
  if (path.length() && *path.begin() != PATH_SEPARATOR)
#endif
    {
      return basePath + PATH_SEPARATOR + path;
    }
    else
    {
      return path;
    }
  }
}

void DefaultFileSystem::SetBasePath(const std::string& path)
{
  basePath = path;

  if ((*basePath.rbegin() == PATH_SEPARATOR))
  {
    basePath.resize(basePath.size() - 1);
  }
}

