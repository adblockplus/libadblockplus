/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#else
#include <sys/stat.h>
#include <cerrno>
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

#ifdef WIN32
  // Paths need to be converted from UTF-8 to UTF-16 on Windows.
  std::wstring NormalizePath(const std::string& path)
  {
    return Utils::ToUtf16String(path);
  }

  #define rename _wrename
  #define remove _wremove
#else
  // POSIX systems: assume that file system encoding is UTF-8 and just use the
  // file paths as they are.
  std::string NormalizePath(const std::string& path)
  {
    return path;
  }
#endif
}

std::shared_ptr<std::istream>
DefaultFileSystem::Read(const std::string& path) const
{
  std::shared_ptr<std::istream> result(new std::ifstream(NormalizePath(path).c_str()));
  if (result->fail())
    throw RuntimeErrorWithErrno("Failed to open " + path);
  return result;
}

void DefaultFileSystem::Write(const std::string& path,
                              std::shared_ptr<std::istream> data)
{
  std::ofstream file(NormalizePath(path).c_str(), std::ios_base::out | std::ios_base::binary);
  file << Utils::Slurp(*data);
}

void DefaultFileSystem::Move(const std::string& fromPath,
                             const std::string& toPath)
{
  if (rename(NormalizePath(fromPath).c_str(), NormalizePath(toPath).c_str()))
    throw RuntimeErrorWithErrno("Failed to move " + fromPath + " to " + toPath);
}

void DefaultFileSystem::Remove(const std::string& path)
{
  if (remove(NormalizePath(path).c_str()))
    throw RuntimeErrorWithErrno("Failed to remove " + path);
}

FileSystem::StatResult DefaultFileSystem::Stat(const std::string& path) const
{
  FileSystem::StatResult result;
#ifdef WIN32
  WIN32_FILE_ATTRIBUTE_DATA data;
  if (!GetFileAttributesExW(NormalizePath(path).c_str(), GetFileExInfoStandard, &data))
  {
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND ||
        err == ERROR_PATH_NOT_FOUND ||
        err == ERROR_INVALID_DRIVE)
    {
      return result;
    }
    throw RuntimeErrorWithErrno("Unable to stat " + path);
  }

  result.exists = true;
  if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    result.isFile = false;
    result.isDirectory = true;
  }
  else
  {
    result.isFile = true;
    result.isDirectory = false;
  }

  // See http://support.microsoft.com/kb/167296 on this conversion
  #define FILE_TIME_TO_UNIX_EPOCH_OFFSET 116444736000000000LL
  #define FILE_TIME_TO_MILLISECONDS_FACTOR 10000
  ULARGE_INTEGER time;
  time.LowPart = data.ftLastWriteTime.dwLowDateTime;
  time.HighPart = data.ftLastWriteTime.dwHighDateTime;
  result.lastModified = (time.QuadPart - FILE_TIME_TO_UNIX_EPOCH_OFFSET) /
      FILE_TIME_TO_MILLISECONDS_FACTOR;
  return result;
#else
  struct stat nativeStat;
  const int failure = stat(NormalizePath(path).c_str(), &nativeStat);
  if (failure)
  {
    if (errno == ENOENT)
      return result;
    throw RuntimeErrorWithErrno("Unable to stat " + path);
  }
  result.exists = true;
  result.isFile = S_ISREG(nativeStat.st_mode);
  result.isDirectory = S_ISDIR(nativeStat.st_mode);

  #define MSEC_IN_SEC 1000
  #define NSEC_IN_MSEC 1000000
  // Note: _POSIX_C_SOURCE macro is defined automatically on Linux due to g++
  // defining _GNU_SOURCE macro. On OS X we still fall back to the "no
  // milliseconds" branch, it has st_mtimespec instead of st_mtim.
#if _POSIX_C_SOURCE >= 200809L
  result.lastModified = static_cast<int64_t>(nativeStat.st_mtim.tv_sec) * MSEC_IN_SEC
                      +  static_cast<int64_t>(nativeStat.st_mtim.tv_nsec) / NSEC_IN_MSEC;
#else
  result.lastModified = static_cast<int64_t>(nativeStat.st_mtime) * MSEC_IN_SEC;
#endif
  return result;
#endif
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
  if (PathIsRelative(NormalizePath(path).c_str()))
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

  if (*basePath.rbegin() == PATH_SEPARATOR)
  {
    basePath.resize(basePath.size() - 1);
  }
}

