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
               std::tr1::shared_ptr<std::istream> data);
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
