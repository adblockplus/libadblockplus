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

#include <sstream>
#include <AdblockPlus.h>
#include <gtest/gtest.h>

namespace
{
  const std::string testPath = "libadblockplus-t\xc3\xa4st-file";

  void WriteString(AdblockPlus::FileSystem& fileSystem,
                   const std::string& content)
  {
    std::shared_ptr<std::stringstream> input(new std::stringstream);
    *input << content;
    fileSystem.Write(testPath, input);
  }
}

TEST(DefaultFileSystemTest, WriteReadRemove)
{
  AdblockPlus::DefaultFileSystem fileSystem;
  WriteString(fileSystem, "foo");
  std::stringstream output;
  output << fileSystem.Read(testPath)->rdbuf();
  fileSystem.Remove(testPath);
  ASSERT_EQ("foo", output.str());
}

TEST(DefaultFileSystemTest, StatWorkingDirectory)
{
  AdblockPlus::DefaultFileSystem fileSystem;
  const AdblockPlus::FileSystem::StatResult result = fileSystem.Stat(".");
  ASSERT_TRUE(result.exists);
  ASSERT_TRUE(result.isDirectory);
  ASSERT_FALSE(result.isFile);
  ASSERT_NE(0, result.lastModified);
}

TEST(DefaultFileSystemTest, WriteMoveStatRemove)
{
  AdblockPlus::DefaultFileSystem fileSystem;
  WriteString(fileSystem, "foo");
  AdblockPlus::FileSystem::StatResult result = fileSystem.Stat(testPath);
  ASSERT_TRUE(result.exists);
  ASSERT_TRUE(result.isFile);
  ASSERT_FALSE(result.isDirectory);
  ASSERT_NE(0, result.lastModified);
  const std::string newTestPath = testPath + "-new";
  fileSystem.Move(testPath, newTestPath);
  result = fileSystem.Stat(testPath);
  ASSERT_FALSE(result.exists);
  result = fileSystem.Stat(newTestPath);
  ASSERT_TRUE(result.exists);
  fileSystem.Remove(newTestPath);
  result = fileSystem.Stat(newTestPath);
  ASSERT_FALSE(result.exists);
}
