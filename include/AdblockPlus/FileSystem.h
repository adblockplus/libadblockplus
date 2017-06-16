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

#ifndef ADBLOCK_PLUS_FILE_SYSTEM_H
#define ADBLOCK_PLUS_FILE_SYSTEM_H

#include <istream>
#include <stdint.h>
#include <string>
#include <memory>

#include "IFileSystem.h"

namespace AdblockPlus
{
  /**
   * File system interface.
   */
  class FileSystem
  {
  public:
    virtual ~FileSystem() {}

    /**
     * Reads from a file.
     * @param path File path.
     * @return Buffer with the file content.
     */
    virtual IFileSystem::IOBuffer Read(const std::string& path) const = 0;

    /**
     * Writes to a file.
     * @param path File path.
     * @param data Buffer with the data to write.
     */
    virtual void Write(const std::string& path,
                       const IFileSystem::IOBuffer& data) = 0;

    /**
     * Moves a file (i.e.\ renames it).
     * @param fromPath Current path to the file.
     * @param toPath New path to the file.
     */
    virtual void Move(const std::string& fromPath,
                      const std::string& toPath) = 0;

    /**
     * Removes a file.
     * @param path File path.
     */
    virtual void Remove(const std::string& path) = 0;

    /**
     * Retrieves information about a file.
     * @param path File path.
     * @return File information.
     */
    virtual IFileSystem::StatResult Stat(const std::string& path) const = 0;

    /**
     * Returns the absolute path to a file.
     * @param path File path (can be relative or absolute).
     * @return Absolute file path.
     */
    virtual std::string Resolve(const std::string& path) const = 0;
  };

  /**
   * Shared smart pointer to a `FileSystem` instance.
   */
  typedef std::shared_ptr<FileSystem> FileSystemSyncPtr;
}

#endif
