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
    std::string lastWrittenPath;
    IOBuffer lastWrittenContent;
    std::string movedFrom;
    std::string movedTo;
    std::string removedPath;
    mutable std::string statPath;
    bool statExists;
    bool statIsDirectory;
    bool statIsFile;
    int statLastModified;

    MockFileSystem() : success(true)
    {
    }

    void Read(const std::string& path, const ReadCallback& callback) const override
    {
      if (!success)
      {
        callback(IOBuffer(), "Unable to read " + path);
        return;
      }
      callback(IOBuffer(contentToRead), "");
    }

    void Write(const std::string& path, const IOBuffer& data,
               const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to write to " + path);
        return;
      }
      lastWrittenPath = path;

      lastWrittenContent = data;
      callback("");
    }

    void Move(const std::string& fromPath, const std::string& toPath,
              const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to move " + fromPath + " to " + toPath);
        return;
      }
      movedFrom = fromPath;
      movedTo = toPath;
      callback("");
    }

    void Remove(const std::string& path, const Callback& callback) override
    {
      if (!success)
      {
        callback("Unable to remove " + path);
        return;
      }
      removedPath = path;
      callback("");
    }

    void Stat(const std::string& path, const StatCallback& callback) const override
    {
      StatResult result;
      std::string error;
      if (!success)
        error = "Unable to stat " + path;
      else
      {
        statPath = path;
        result.exists = statExists;
        result.isDirectory = statIsDirectory;
        result.isFile = statIsFile;
        result.lastModified = statLastModified;
      }
      callback(result, error);
    }

    std::string Resolve(const std::string& path) const override
    {
      if (!success)
        throw std::runtime_error("Unable to stat " + path);
      return path;
    }
  };

  void ReadFile(AdblockPlus::JsEnginePtr jsEngine, std::string& content,
                std::string& error)
  {
    jsEngine->Evaluate("_fileSystem.read('', function(r) {result = r})");
    AdblockPlus::Sleep(50);
    content = jsEngine->Evaluate("result.content").AsString();
    error = jsEngine->Evaluate("result.error").AsString();
  }

  typedef std::shared_ptr<MockFileSystem> MockFileSystemPtr;

  class FileSystemJsObjectTest : public BaseJsTest
  {
  protected:
    MockFileSystemPtr mockFileSystem;

    void SetUp()
    {
      mockFileSystem = MockFileSystemPtr(new MockFileSystem);
      JsEngineCreationParameters params;
      params.fileSystem = mockFileSystem;
      jsEngine = CreateJsEngine(std::move(params));
    }
  };
}

TEST_F(FileSystemJsObjectTest, Read)
{
  mockFileSystem->contentToRead =
    AdblockPlus::IFileSystem::IOBuffer{'f', 'o', 'o'};
  std::string content;
  std::string error;
  ReadFile(jsEngine, content, error);
  ASSERT_EQ("foo", content);
  ASSERT_EQ("undefined", error);
}

TEST_F(FileSystemJsObjectTest, ReadIllegalArguments)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.read()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.read('', '')"));
}

TEST_F(FileSystemJsObjectTest, ReadError)
{
  mockFileSystem->success = false;
  std::string content;
  std::string error;
  ReadFile(jsEngine, content, error);
  ASSERT_NE("", error);
  ASSERT_EQ("", content);
}

TEST_F(FileSystemJsObjectTest, Write)
{
  jsEngine->Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_EQ("foo", mockFileSystem->lastWrittenPath);
  ASSERT_EQ((AdblockPlus::IFileSystem::IOBuffer{'b', 'a', 'r'}),
            mockFileSystem->lastWrittenContent);
  ASSERT_TRUE(jsEngine->Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, WriteIllegalArguments)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.write()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.write('', '', '')"));
}

TEST_F(FileSystemJsObjectTest, WriteError)
{
  mockFileSystem->success = false;
  jsEngine->Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_NE("", jsEngine->Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Move)
{
  jsEngine->Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_EQ("foo", mockFileSystem->movedFrom);
  ASSERT_EQ("bar", mockFileSystem->movedTo);
  ASSERT_TRUE(jsEngine->Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, MoveIllegalArguments)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.move()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.move('', '', '')"));
}

TEST_F(FileSystemJsObjectTest, MoveError)
{
  mockFileSystem->success = false;
  jsEngine->Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_FALSE(jsEngine->Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, Remove)
{
  jsEngine->Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_EQ("foo", mockFileSystem->removedPath);
  ASSERT_TRUE(jsEngine->Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, RemoveIllegalArguments)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.remove()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.remove('', '')"));
}

TEST_F(FileSystemJsObjectTest, RemoveError)
{
  mockFileSystem->success = false;
  jsEngine->Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  AdblockPlus::Sleep(50);
  ASSERT_NE("", jsEngine->Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Stat)
{
  mockFileSystem->statExists = true;
  mockFileSystem->statIsDirectory= false;
  mockFileSystem->statIsFile = true;
  mockFileSystem->statLastModified = 1337;
  jsEngine->Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  AdblockPlus::Sleep(50);
  ASSERT_EQ("foo", mockFileSystem->statPath);
  ASSERT_TRUE(jsEngine->Evaluate("result.error").IsUndefined());
  ASSERT_TRUE(jsEngine->Evaluate("result.exists").AsBool());
  ASSERT_FALSE(jsEngine->Evaluate("result.isDirectory").AsBool());
  ASSERT_TRUE(jsEngine->Evaluate("result.isFile").AsBool());
  ASSERT_EQ(1337, jsEngine->Evaluate("result.lastModified").AsInt());
}

TEST_F(FileSystemJsObjectTest, StatIllegalArguments)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.stat()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_fileSystem.stat('', '')"));
}

TEST_F(FileSystemJsObjectTest, StatError)
{
  mockFileSystem->success = false;
  jsEngine->Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  AdblockPlus::Sleep(50);
  ASSERT_FALSE(jsEngine->Evaluate("result.error").IsUndefined());
}
