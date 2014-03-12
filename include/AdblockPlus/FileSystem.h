/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2014 Eyeo GmbH
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
      StatResult()
      {
        exists = false;
        isDirectory = false;
        isFile = false;
        lastModified = 0;
      }

      bool exists;
      bool isDirectory;
      bool isFile;
      int64_t lastModified;
    };

    virtual ~FileSystem() {}
    virtual std::tr1::shared_ptr<std::istream>
      Read(const std::string& path) const = 0;
    virtual void Write(const std::string& path,
                       std::tr1::shared_ptr<std::istream> data) = 0;
    virtual void Move(const std::string& fromPath,
                      const std::string& toPath) = 0;
    virtual void Remove(const std::string& path) = 0;
    virtual StatResult Stat(const std::string& path) const = 0;
    virtual std::string Resolve(const std::string& path) const = 0;
  };

  typedef std::tr1::shared_ptr<FileSystem> FileSystemPtr;
}

#endif
