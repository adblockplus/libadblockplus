/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

namespace AdblockPlus
{
  /**
   * File system interface.
   */
  class FileSystem
  {
  public:
    /**
     * Result of a stat operation, i.e.\ information about a file.
     */
    struct StatResult
    {
      StatResult()
      {
        exists = false;
        isDirectory = false;
        isFile = false;
        lastModified = 0;
      }

      /**
       * File exists.
       */
      bool exists;

      /**
       * File is a directory.
       */
      bool isDirectory;

      /**
       * File is a regular file.
       */
      bool isFile;

      /**
       * POSIX time of the last modification.
       */
      int64_t lastModified;
    };

    virtual ~FileSystem() {}

    /**
     * Reads from a file.
     * @param path File path.
     * @return Input stream with the file's contents.
     */
    virtual std::shared_ptr<std::istream>
      Read(const std::string& path) const = 0;

    /**
     * Writes to a file.
     * @param path File path.
     * @param data Input stream with the data to write.
     */
    virtual void Write(const std::string& path,
                       std::shared_ptr<std::istream> data) = 0;

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
    virtual StatResult Stat(const std::string& path) const = 0;

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
  typedef std::shared_ptr<FileSystem> FileSystemPtr;
}

#endif
