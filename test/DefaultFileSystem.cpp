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

#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>

#include "../src/DefaultFileSystem.h"
#include "../src/DefaultResourceReader.h"
#include "BaseJsTest.h"

using AdblockPlus::FileSystemPtr;
using AdblockPlus::IFileSystem;
using AdblockPlus::SchedulerTask;

using namespace AdblockPlus;
namespace
{
  const std::string testFileName = "libadblockplus-t\xc3\xa4st-file";

  FileSystemPtr CreateDefaultFileSystem(const Scheduler& scheduler)
  {
    return FileSystemPtr(new DefaultFileSystem(
        scheduler, std::unique_ptr<DefaultFileSystemSync>(new DefaultFileSystemSync(""))));
  }

  class DefaultFileSystemTest : public ::testing::Test
  {
  public:
    void SetUp() override
    {
      fileSystem = CreateDefaultFileSystem([this](const SchedulerTask& task) {
        fileSystemTasks.emplace_back(task);
      });
    }

  protected:
    void WriteString(const std::string& content)
    {
      bool hasRun = false;
      fileSystem->Write(testFileName,
                        IFileSystem::IOBuffer(content.cbegin(), content.cend()),
                        [&hasRun](const std::string& error) {
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

#ifdef _WIN32
#define SLASH_STRING "\\"
#else
#define SLASH_STRING "/"
#endif

TEST(DefaultFileSystemBasePathTest, BasePathAndResolveTest)
{
  class TestFSSync : public DefaultFileSystemSync
  {
  public:
    explicit TestFSSync(const std::string& basePath) : DefaultFileSystemSync(basePath)
    {
    }
    const std::string& base() const
    {
      return basePath;
    }
  };

  {
    auto fs = std::unique_ptr<TestFSSync>(new TestFSSync(""));
    EXPECT_EQ("", fs->base());
    std::string fullPath = fs->Resolve("bar" SLASH_STRING "baz.txt");
    EXPECT_EQ("bar" SLASH_STRING "baz.txt", fullPath);
  }
  {
    auto fs = std::unique_ptr<TestFSSync>(new TestFSSync(SLASH_STRING));
    EXPECT_EQ(SLASH_STRING, fs->base());
    std::string fullPath = fs->Resolve("bar" SLASH_STRING "baz.txt");
    EXPECT_EQ(SLASH_STRING "bar" SLASH_STRING "baz.txt", fullPath);
  }
  {
    auto fs = std::unique_ptr<TestFSSync>(new TestFSSync(SLASH_STRING "foo" SLASH_STRING));
    EXPECT_EQ(SLASH_STRING "foo", fs->base());
    std::string fullPath = fs->Resolve("bar" SLASH_STRING "baz.txt");
    EXPECT_EQ(SLASH_STRING "foo" SLASH_STRING "bar" SLASH_STRING "baz.txt", fullPath);
  }
  {
    auto fs = std::unique_ptr<TestFSSync>(new TestFSSync(SLASH_STRING "foo"));
    EXPECT_EQ(SLASH_STRING "foo", fs->base());
    std::string fullPath = fs->Resolve("bar" SLASH_STRING "baz.txt");
    EXPECT_EQ(SLASH_STRING "foo" SLASH_STRING "bar" SLASH_STRING "baz.txt", fullPath);
  }
}

TEST_F(DefaultFileSystemTest, WriteReadRemove)
{
  WriteString("foo");

  bool hasReadRun = false;
  bool hasErrorRun = false;
  fileSystem->Read(
      testFileName,
      [&hasReadRun](IFileSystem::IOBuffer&& content) {
        EXPECT_EQ("foo", std::string(content.cbegin(), content.cend()));
        hasReadRun = true;
      },
      [&hasErrorRun](const std::string& error) {
        hasErrorRun = true;
      });
  EXPECT_FALSE(hasReadRun);
  EXPECT_FALSE(hasErrorRun);
  PumpTask();
  EXPECT_TRUE(hasReadRun);
  EXPECT_FALSE(hasErrorRun);

  bool hasRemoveRun = false;
  fileSystem->Remove(testFileName, [&hasRemoveRun](const std::string& error) {
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
                   [&hasStatRun](const IFileSystem::StatResult result, const std::string& error) {
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
  fileSystem->Stat(
      testFileName,
      [&hasStatOrigFileExistsRun](const IFileSystem::StatResult& result, const std::string& error) {
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
  fileSystem->Move(
      testFileName, newTestFileName, [&hasMoveRun, newTestFileName](const std::string& error) {
        EXPECT_TRUE(error.empty());
        hasMoveRun = true;
      });
  EXPECT_FALSE(hasMoveRun);
  PumpTask();
  EXPECT_TRUE(hasMoveRun);

  bool hasStatOrigFileDontExistsRun = false;
  fileSystem->Stat(testFileName,
                   [&hasStatOrigFileDontExistsRun](const IFileSystem::StatResult& result,
                                                   const std::string& error) {
                     EXPECT_TRUE(error.empty());
                     ASSERT_FALSE(result.exists);
                     hasStatOrigFileDontExistsRun = true;
                   });
  EXPECT_FALSE(hasStatOrigFileDontExistsRun);
  PumpTask();
  EXPECT_TRUE(hasStatOrigFileDontExistsRun);

  bool hasStatNewFileExistsRun = false;
  fileSystem->Stat(
      newTestFileName,
      [&hasStatNewFileExistsRun](const IFileSystem::StatResult& result, const std::string& error) {
        EXPECT_TRUE(error.empty());
        ASSERT_TRUE(result.exists);
        hasStatNewFileExistsRun = true;
      });
  EXPECT_FALSE(hasStatNewFileExistsRun);
  PumpTask();
  EXPECT_TRUE(hasStatNewFileExistsRun);

  bool hasRemoveRun = false;
  fileSystem->Remove(newTestFileName, [&hasRemoveRun](const std::string& error) {
    EXPECT_TRUE(error.empty());
    hasRemoveRun = true;
  });
  EXPECT_FALSE(hasRemoveRun);
  PumpTask();
  EXPECT_TRUE(hasRemoveRun);

  bool hasStatRemovedFileRun = false;
  fileSystem->Stat(
      newTestFileName,
      [&hasStatRemovedFileRun](const IFileSystem::StatResult& result, const std::string& error) {
        EXPECT_TRUE(error.empty());
        ASSERT_FALSE(result.exists);
        hasStatRemovedFileRun = true;
      });
  EXPECT_FALSE(hasStatRemovedFileRun);
  PumpTask();
  EXPECT_TRUE(hasStatRemovedFileRun);
}

TEST_F(DefaultFileSystemTest, ResetAfterCallbackScheduled)
{
  AdblockPlus::AppInfo appInfo;
  appInfo.version = "1.0";
  appInfo.name = "abpshell";
  appInfo.application = "standalone";
  appInfo.applicationVersion = "1.0";
  appInfo.locale = "en-US";
  AdblockPlus::PlatformFactory::CreationParameters platformParams;
  platformParams.timer.reset(new NoopTimer());
  platformParams.webRequest.reset(new NoopWebRequest());
  platformParams.logSystem.reset(new LazyLogSystem());
  platformParams.resourceReader.reset(new DefaultResourceReader());
  platformParams.fileSystem.reset(fileSystem.release());

  auto platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(platformParams));
  platform->SetUp(appInfo);
  AdblockPlus::JsEngine& jsEngine =
      static_cast<AdblockPlus::DefaultPlatform*>(platform.get())->GetJsEngine();

  jsEngine.Evaluate("let result = {}; _fileSystem.read('', function(r) {result.content = "
                    "r.content;}, function(error) {result.error = error;})");
  // Normally the line below makes sure all the scheduled callbacks are executed
  // and no new can be scheduled. However if JsValue is bound to the callback
  // passed to embedder it may  outlive the platform making the destruction order
  // incorrect. This should not happen due to JsWeakValues usage; we do not
  // bind JsValues to the callbacks passed to embedder but a "weak reference" to
  // them and the weak references go away with JsEngine so the destruction order is correct.
  // However there was a bug in file system making weak values not weak and without the fix this
  // crashed.
  platform.reset();
}
