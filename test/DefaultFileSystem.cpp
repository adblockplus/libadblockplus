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

#include <sstream>
#include <AdblockPlus.h>
#include <gtest/gtest.h>

#include "BaseJsTest.h"

using AdblockPlus::IFileSystem;
using AdblockPlus::Sync;

namespace
{
  const std::string testPath = "libadblockplus-t\xc3\xa4st-file";

  void WriteString(const AdblockPlus::FileSystemPtr& fileSystem,
                   const std::string& content)
  {
    Sync sync;

    fileSystem->Write(testPath,
      IFileSystem::IOBuffer(content.cbegin(), content.cend()),
      [&sync](const std::string& error)
      {
        EXPECT_TRUE(error.empty());

        sync.Set();
      });

    sync.WaitFor();
  }
}

TEST(DefaultFileSystemTest, WriteReadRemove)
{
  Sync sync;
  AdblockPlus::FileSystemPtr fileSystem = AdblockPlus::CreateDefaultFileSystem();
  WriteString(fileSystem, "foo");
  fileSystem->Read(testPath,
    [fileSystem, &sync](IFileSystem::IOBuffer&& content, const std::string& error)
    {
      EXPECT_TRUE(error.empty());
      EXPECT_EQ("foo", std::string(content.cbegin(), content.cend()));

      fileSystem->Remove(testPath, [&sync](const std::string& error)
        {
          EXPECT_TRUE(error.empty());
          sync.Set();
        });
    });

  EXPECT_TRUE(sync.WaitFor());
}

TEST(DefaultFileSystemTest, StatWorkingDirectory)
{
  Sync sync;
  AdblockPlus::FileSystemPtr fileSystem = AdblockPlus::CreateDefaultFileSystem();
  fileSystem->Stat(".",
    [fileSystem, &sync](const IFileSystem::StatResult result, const std::string& error)
    {
      EXPECT_TRUE(error.empty());
      ASSERT_TRUE(result.exists);
      ASSERT_TRUE(result.isDirectory);
      ASSERT_FALSE(result.isFile);
      ASSERT_NE(0, result.lastModified);
      sync.Set();
    });

  EXPECT_TRUE(sync.WaitFor());
}

TEST(DefaultFileSystemTest, WriteMoveStatRemove)
{
  Sync sync;
  AdblockPlus::FileSystemPtr fileSystem = AdblockPlus::CreateDefaultFileSystem();
  WriteString(fileSystem, "foo");

  fileSystem->Stat(testPath,
    [fileSystem, &sync](const IFileSystem::StatResult& result, const std::string& error)
    {
      EXPECT_TRUE(error.empty());
      ASSERT_TRUE(result.exists);
      ASSERT_TRUE(result.isFile);
      ASSERT_FALSE(result.isDirectory);
      ASSERT_NE(0, result.lastModified);
      const std::string newTestPath = testPath + "-new";
      fileSystem->Move(testPath, newTestPath, [fileSystem, &sync, newTestPath](const std::string& error)
      {
        EXPECT_TRUE(error.empty());
        fileSystem->Stat(testPath, [fileSystem, &sync, newTestPath](const IFileSystem::StatResult& result, const std::string& error)
        {
          EXPECT_TRUE(error.empty());
          ASSERT_FALSE(result.exists);
          fileSystem->Stat(newTestPath, [fileSystem, &sync, newTestPath](const IFileSystem::StatResult& result, const std::string& error)
          {
            EXPECT_TRUE(error.empty());
            ASSERT_TRUE(result.exists);
            fileSystem->Remove(newTestPath, [fileSystem, &sync, newTestPath](const std::string& error)
            {
              EXPECT_TRUE(error.empty());
              fileSystem->Stat(newTestPath, [fileSystem, &sync, newTestPath](const IFileSystem::StatResult& result, const std::string& error)
              {
                EXPECT_TRUE(error.empty());
                ASSERT_FALSE(result.exists);
                sync.Set();
              });
            });
          });
        });
      });
    });

  EXPECT_TRUE(sync.WaitFor());
}
