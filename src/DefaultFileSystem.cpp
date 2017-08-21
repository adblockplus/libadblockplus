/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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

#include "DefaultFileSystem.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

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

IFileSystem::IOBuffer
DefaultFileSystemSync::Read(const std::string& path) const
{
  std::ifstream file(NormalizePath(path).c_str(), std::ios_base::binary);
  if (file.fail())
    throw RuntimeErrorWithErrno("Failed to open " + path);

  file.seekg(0, std::ios_base::end);
  auto dataSize = file.tellg();
  file.seekg(0, std::ios_base::beg);

  IFileSystem::IOBuffer data(dataSize);
  file.read(reinterpret_cast<std::ifstream::char_type*>(data.data()),
            data.size());
  return data;
}

void DefaultFileSystemSync::Write(const std::string& path,
                              const IFileSystem::IOBuffer& data)
{
  std::ofstream file(NormalizePath(path).c_str(), std::ios_base::out | std::ios_base::binary);
  file.write(reinterpret_cast<const std::ofstream::char_type*>(data.data()),
             data.size());
}

void DefaultFileSystemSync::Move(const std::string& fromPath,
                                 const std::string& toPath)
{
  if (rename(NormalizePath(fromPath).c_str(), NormalizePath(toPath).c_str()))
    throw RuntimeErrorWithErrno("Failed to move " + fromPath + " to " + toPath);
}

void DefaultFileSystemSync::Remove(const std::string& path)
{
  if (remove(NormalizePath(path).c_str()))
    throw RuntimeErrorWithErrno("Failed to remove " + path);
}

IFileSystem::StatResult DefaultFileSystemSync::Stat(const std::string& path) const
{
  IFileSystem::StatResult result;
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

std::string DefaultFileSystemSync::Resolve(const std::string& path) const
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

void DefaultFileSystemSync::SetBasePath(const std::string& path)
{
  basePath = path;

  if (*basePath.rbegin() == PATH_SEPARATOR)
  {
    basePath.resize(basePath.size() - 1);
  }
}

DefaultFileSystem::DefaultFileSystem(const Scheduler& scheduler, std::unique_ptr<DefaultFileSystemSync> syncImpl)
  : scheduler(scheduler), syncImpl(std::move(syncImpl))
{
}

void DefaultFileSystem::Read(const std::string& fileName,
                             const ReadCallback& callback) const
{
  scheduler([this, fileName, callback]
  {
    std::string error;
    try
    {
      auto data = syncImpl->Read(Resolve(fileName));
      callback(std::move(data), error);
      return;
    }
    catch (std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error =  "Unknown error while reading from " + fileName + " as " + Resolve(fileName);
    }
    callback(IOBuffer(), error);
  });
}

void DefaultFileSystem::Write(const std::string& fileName,
                              const IOBuffer& data,
                              const Callback& callback)
{
  scheduler([this, fileName, data, callback]
  {
    std::string error;
    try
    {
      syncImpl->Write(Resolve(fileName), data);
    }
    catch (std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Unknown error while writing to " + fileName + " as " + Resolve(fileName);
    }
    callback(error);
  });
}

void DefaultFileSystem::Move(const std::string& fromFileName,
                             const std::string& toFileName,
                             const Callback& callback)
{
  scheduler([this, fromFileName, toFileName, callback]
  {
    std::string error;
    try
    {
      syncImpl->Move(Resolve(fromFileName), Resolve(toFileName));
    }
    catch (std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Unknown error while moving " + fromFileName + " to " + toFileName;
    }
    callback(error);
  });
}

void DefaultFileSystem::Remove(const std::string& fileName,
                               const Callback& callback)
{
  scheduler([this, fileName, callback]
  {
    std::string error;
    try
    {
      syncImpl->Remove(Resolve(fileName));
    }
    catch (std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Unknown error while removing " + fileName + " as " + Resolve(fileName);
    }
    callback(error);
  });
}

void DefaultFileSystem::Stat(const std::string& fileName,
                             const StatCallback& callback) const
{
  scheduler([this, fileName, callback]
  {
    std::string error;
    try
    {
      auto result = syncImpl->Stat(Resolve(fileName));
      callback(result, error);
      return;
    }
    catch (std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "Unknown error while calling stat on " + fileName + " as " + Resolve(fileName);
    }
    callback(StatResult(), error);
  });
}

std::string DefaultFileSystem::Resolve(const std::string& fileName) const
{
  return syncImpl->Resolve(fileName);
}
