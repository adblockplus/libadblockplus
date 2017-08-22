/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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
using AdblockPlus::FileSystemPtr;
using AdblockPlus::SchedulerTask;

namespace
{
  const std::string testFileName = "libadblockplus-t\xc3\xa4st-file";

  class DefaultFileSystemTest : public ::testing::Test
  {
  public:
    void SetUp() override
    {
      fileSystem = AdblockPlus::CreateDefaultFileSystem([this](const SchedulerTask& task)
      {
        fileSystemTasks.emplace_back(task);
      });
    }
  protected:
    void WriteString(const std::string& content)
    {
      bool hasRun = false;
      fileSystem->Write(testFileName,
        IFileSystem::IOBuffer(content.cbegin(), content.cend()),
        [&hasRun](const std::string& error)
      {
        EXPECT_TRUE(error.empty()) << error;
        hasRun = true;
      });
      EXPECT_FALSE(hasRun);
      PumpTask();
      EXPECT_TRUE(hasRun);
    }

    void PumpTask()
    {
      ASSERT_EQ(1u, fileSystemTasks.size());
      (*fileSystemTasks.begin())();
      fileSystemTasks.pop_front();
    }

    std::list<SchedulerTask> fileSystemTasks;
    FileSystemPtr fileSystem;
  };
}

TEST_F(DefaultFileSystemTest, WriteReadRemove)
{
  WriteString("foo");

  bool hasReadRun = false;
  fileSystem->Read(testFileName,
    [this, &hasReadRun](IFileSystem::IOBuffer&& content, const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    EXPECT_EQ("foo", std::string(content.cbegin(), content.cend()));
    hasReadRun = true;
  });
  EXPECT_FALSE(hasReadRun);
  PumpTask();
  EXPECT_TRUE(hasReadRun);

  bool hasRemoveRun = false;
  fileSystem->Remove(testFileName, [&hasRemoveRun](const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    hasRemoveRun = true;
  });
  EXPECT_FALSE(hasRemoveRun);
  PumpTask();
  EXPECT_TRUE(hasRemoveRun);
}

TEST_F(DefaultFileSystemTest, StatWorkingDirectory)
{
  bool hasStatRun = false;
  fileSystem->Stat(".",
    [&hasStatRun](const IFileSystem::StatResult result, const std::string& error)
    {
      EXPECT_TRUE(error.empty());
      ASSERT_TRUE(result.exists);
      ASSERT_NE(0, result.lastModified);
      hasStatRun = true;
    });
  EXPECT_FALSE(hasStatRun);
  PumpTask();
  EXPECT_TRUE(hasStatRun);
}

TEST_F(DefaultFileSystemTest, WriteMoveStatRemove)
{
  WriteString("foo");

  bool hasStatOrigFileExistsRun = false;
  fileSystem->Stat(testFileName,
    [&hasStatOrigFileExistsRun](const IFileSystem::StatResult& result, const std::string& error)
    {
      EXPECT_TRUE(error.empty());
      ASSERT_TRUE(result.exists);
      ASSERT_NE(0, result.lastModified);
      hasStatOrigFileExistsRun = true;
    });
  EXPECT_FALSE(hasStatOrigFileExistsRun);
  PumpTask();
  EXPECT_TRUE(hasStatOrigFileExistsRun);

  const std::string newTestFileName = testFileName + "-new";
  bool hasMoveRun = false;
  fileSystem->Move(testFileName, newTestFileName, [&hasMoveRun, newTestFileName](const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    hasMoveRun = true;
  });
  EXPECT_FALSE(hasMoveRun);
  PumpTask();
  EXPECT_TRUE(hasMoveRun);

  bool hasStatOrigFileDontExistsRun = false;
  fileSystem->Stat(testFileName, [&hasStatOrigFileDontExistsRun](const IFileSystem::StatResult& result, const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    ASSERT_FALSE(result.exists);
    hasStatOrigFileDontExistsRun = true;
  });
  EXPECT_FALSE(hasStatOrigFileDontExistsRun);
  PumpTask();
  EXPECT_TRUE(hasStatOrigFileDontExistsRun);

  bool hasStatNewFileExistsRun = false;
  fileSystem->Stat(newTestFileName, [&hasStatNewFileExistsRun](const IFileSystem::StatResult& result, const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    ASSERT_TRUE(result.exists);
    hasStatNewFileExistsRun = true;
  });
  EXPECT_FALSE(hasStatNewFileExistsRun);
  PumpTask();
  EXPECT_TRUE(hasStatNewFileExistsRun);

  bool hasRemoveRun = false;
  fileSystem->Remove(newTestFileName, [&hasRemoveRun](const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    hasRemoveRun = true;
  });
  EXPECT_FALSE(hasRemoveRun);
  PumpTask();
  EXPECT_TRUE(hasRemoveRun);

  bool hasStatRemovedFileRun = false;
  fileSystem->Stat(newTestFileName, [&hasStatRemovedFileRun](const IFileSystem::StatResult& result, const std::string& error)
  {
    EXPECT_TRUE(error.empty());
    ASSERT_FALSE(result.exists);
    hasStatRemovedFileRun = true;
  });
  EXPECT_FALSE(hasStatRemovedFileRun);
  PumpTask();
  EXPECT_TRUE(hasStatRemovedFileRun);
}
