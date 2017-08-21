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
#include "BaseJsTest.h"
#include "../src/Thread.h"

namespace
{
  class MockFileSystem : public AdblockPlus::IFileSystem
  {
  public:
    bool success;
    IOBuffer contentToRead;
    std::string lastWrittenFile;
    IOBuffer lastWrittenContent;
    std::string movedFrom;
    std::string movedTo;
    std::string removedFile;
    mutable std::string statFile;
    bool statExists;
    int statLastModified;

    MockFileSystem() : success(true)
    {
    }

    void Read(const std::string& fileName, const ReadCallback& callback) const override
    {
      if (!success)
      {
        callback(IOBuffer(), "Unable to read " + fileName);
        return;
      }
      callback(IOBuffer(contentToRead), "");
    }

    void Write(const std::string& fileName, const IOBuffer& data,
               const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to write to " + fileName);
        return;
      }
      lastWrittenFile = fileName;

      lastWrittenContent = data;
      callback("");
    }

    void Move(const std::string& fromFileName, const std::string& toFileName,
              const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to move " + fromFileName + " to " + toFileName);
        return;
      }
      movedFrom = fromFileName;
      movedTo = toFileName;
      callback("");
    }

    void Remove(const std::string& fileName, const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to remove " + fileName);
        return;
      }
      removedFile = fileName;
      callback("");
    }

    void Stat(const std::string& fileName, const StatCallback& callback) const override
    {
      StatResult result;
      std::string error;
      if (!success)
        error = "Unable to stat " + fileName;
      else
      {
        statFile = fileName;
        result.exists = statExists;
        result.lastModified = statLastModified;
      }
      callback(result, error);
    }
  };

  void ReadFile(AdblockPlus::JsEngine& jsEngine, std::string& content,
                std::string& error)
  {
    jsEngine.Evaluate("_fileSystem.read('', function(r) {result = r})");
    content = jsEngine.Evaluate("result.content").AsString();
    error = jsEngine.Evaluate("result.error").AsString();
  }

  typedef std::shared_ptr<MockFileSystem> MockFileSystemPtr;

  class FileSystemJsObjectTest : public BaseJsTest
  {
  protected:
    MockFileSystemPtr mockFileSystem;

    void SetUp()
    {
      mockFileSystem = MockFileSystemPtr(new MockFileSystem());
      ThrowingPlatformCreationParameters params;
      params.fileSystem = mockFileSystem;
      platform.reset(new AdblockPlus::Platform(std::move(params)));
    }
  };
}

TEST_F(FileSystemJsObjectTest, Read)
{
  mockFileSystem->contentToRead =
    AdblockPlus::IFileSystem::IOBuffer{'f', 'o', 'o'};
  std::string content;
  std::string error;
  ReadFile(GetJsEngine(), content, error);
  ASSERT_EQ("foo", content);
  ASSERT_EQ("undefined", error);
}

TEST_F(FileSystemJsObjectTest, ReadIllegalArguments)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.read()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.read('', '')"));
}

TEST_F(FileSystemJsObjectTest, ReadError)
{
  mockFileSystem->success = false;
  std::string content;
  std::string error;
  ReadFile(GetJsEngine(), content, error);
  ASSERT_NE("", error);
  ASSERT_EQ("", content);
}

TEST_F(FileSystemJsObjectTest, Write)
{
  GetJsEngine().Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  ASSERT_EQ("foo", mockFileSystem->lastWrittenFile);
  ASSERT_EQ((AdblockPlus::IFileSystem::IOBuffer{'b', 'a', 'r'}),
            mockFileSystem->lastWrittenContent);
  ASSERT_TRUE(GetJsEngine().Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, WriteIllegalArguments)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.write()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.write('', '', '')"));
}

TEST_F(FileSystemJsObjectTest, WriteError)
{
  mockFileSystem->success = false;
  GetJsEngine().Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  ASSERT_NE("", GetJsEngine().Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Move)
{
  GetJsEngine().Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  ASSERT_EQ("foo", mockFileSystem->movedFrom);
  ASSERT_EQ("bar", mockFileSystem->movedTo);
  ASSERT_TRUE(GetJsEngine().Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, MoveIllegalArguments)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.move()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.move('', '', '')"));
}

TEST_F(FileSystemJsObjectTest, MoveError)
{
  mockFileSystem->success = false;
  GetJsEngine().Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  ASSERT_FALSE(GetJsEngine().Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, Remove)
{
  GetJsEngine().Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  ASSERT_EQ("foo", mockFileSystem->removedFile);
  ASSERT_TRUE(GetJsEngine().Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, RemoveIllegalArguments)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.remove()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.remove('', '')"));
}

TEST_F(FileSystemJsObjectTest, RemoveError)
{
  mockFileSystem->success = false;
  GetJsEngine().Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  ASSERT_NE("", GetJsEngine().Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Stat)
{
  mockFileSystem->statExists = true;
  mockFileSystem->statLastModified = 1337;
  GetJsEngine().Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  ASSERT_EQ("foo", mockFileSystem->statFile);
  ASSERT_TRUE(GetJsEngine().Evaluate("result.error").IsUndefined());
  ASSERT_TRUE(GetJsEngine().Evaluate("result.exists").AsBool());
  ASSERT_EQ(1337, GetJsEngine().Evaluate("result.lastModified").AsInt());
}

TEST_F(FileSystemJsObjectTest, StatIllegalArguments)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.stat()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("_fileSystem.stat('', '')"));
}

TEST_F(FileSystemJsObjectTest, StatError)
{
  mockFileSystem->success = false;
  GetJsEngine().Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  ASSERT_FALSE(GetJsEngine().Evaluate("result.error").IsUndefined());
}
