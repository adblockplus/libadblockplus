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
#include "BaseJsTest.h"
#include "../src/Thread.h"

using namespace AdblockPlus;

extern std::string jsSources[];

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

    void Read(const std::string& fileName, const ReadCallback& callback, const Callback& errorCallback) const override
    {
      if (success)
        try
        {
          callback(IOBuffer(contentToRead));
        }
        catch (const std::exception& ex)
        {
          errorCallback(ex.what());
        }
      else
        errorCallback("Unable to read " + fileName);
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
    jsEngine.Evaluate("let result = {}; _fileSystem.read('', function(r) {result.content = r.content;}, function(error) {result.error = error;})");
    content = jsEngine.Evaluate("result.content").AsString();
    error = jsEngine.Evaluate("result.error").AsString();
  }

  class FileSystemJsObjectTest : public BaseJsTest
  {
  protected:
    MockFileSystem* mockFileSystem;

    void SetUp()
    {
      ThrowingPlatformCreationParameters params;
      params.fileSystem.reset(mockFileSystem = new MockFileSystem());
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
  EXPECT_NE("", error);
  EXPECT_NE("undefined", error);
  EXPECT_EQ("undefined", content);
}

TEST_F(FileSystemJsObjectTest, Write)
{
  GetJsEngine().Evaluate("let error = true; _fileSystem.write('foo', 'bar', function(e) {error = e})");
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
  GetJsEngine().Evaluate("let error = true; _fileSystem.write('foo', 'bar', function(e) {error = e})");
  ASSERT_NE("", GetJsEngine().Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Move)
{
  GetJsEngine().Evaluate("let error = true; _fileSystem.move('foo', 'bar', function(e) {error = e})");
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
  GetJsEngine().Evaluate("let error; _fileSystem.move('foo', 'bar', function(e) {error = e})");
  ASSERT_FALSE(GetJsEngine().Evaluate("error").IsUndefined());
}

TEST_F(FileSystemJsObjectTest, Remove)
{
  GetJsEngine().Evaluate("let error = true; _fileSystem.remove('foo', function(e) {error = e})");
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
  GetJsEngine().Evaluate("let error = true; _fileSystem.remove('foo', function(e) {error = e})");
  ASSERT_NE("", GetJsEngine().Evaluate("error").AsString());
}

TEST_F(FileSystemJsObjectTest, Stat)
{
  mockFileSystem->statExists = true;
  mockFileSystem->statLastModified = 1337;
  GetJsEngine().Evaluate("let result; _fileSystem.stat('foo', function(r) {result = r})");
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
  GetJsEngine().Evaluate("let result; _fileSystem.stat('foo', function(r) {result = r})");
  ASSERT_FALSE(GetJsEngine().Evaluate("result.error").IsUndefined());
}

namespace
{
  class FileSystemJsObject_ReadFromFileTest : public FileSystemJsObjectTest
  {
  public:
    typedef std::function<void(const std::string&)> ReadFromFileCallback;
    typedef std::vector<std::string> Lines;

    void readFromFile(const ReadFromFileCallback& onLine)
    {
      ASSERT_TRUE(onLine);
      auto& jsEngine = GetJsEngine();
      bool isOnDoneCalled = false;
      jsEngine.SetEventCallback("onLine", [onLine](JsValueList&& /*line*/jsArgs)
      {
        ASSERT_EQ(1u, jsArgs.size());
        EXPECT_TRUE(jsArgs[0].IsString());
        onLine(jsArgs[0].AsString());
      });
      jsEngine.SetEventCallback("onDone", [this, &isOnDoneCalled](JsValueList&& /*error*/jsArgs)
      {
        isOnDoneCalled = true;
        if (this->mockFileSystem->success)
        {
          EXPECT_EQ(0u, jsArgs.size()) << jsArgs[0].AsString();
        }
        else
        {
          ASSERT_EQ(1u, jsArgs.size());
          EXPECT_TRUE(jsArgs[0].IsString());
          EXPECT_FALSE(jsArgs[0].AsString().empty());
        }
      });
      jsEngine.Evaluate(R"js(_fileSystem.readFromFile("foo",
  (line) => _triggerEvent("onLine", line),
  () =>_triggerEvent("onDone"),
  (error) => _triggerEvent("onDone", error));
)js");
      EXPECT_TRUE(isOnDoneCalled);
    }

    void readFromFile_Lines(const std::string& content, const Lines& expected)
    {
      mockFileSystem->contentToRead.assign(content.begin(), content.end());
      std::vector<std::string> readLines;
      readFromFile([&readLines](const std::string& line)
        {
          readLines.emplace_back(line);
        });

      ASSERT_EQ(expected.size(), readLines.size());
      for (Lines::size_type i = 0; i < expected.size(); ++i)
      {
        EXPECT_EQ(expected[i], readLines[i]);
      }
    }
  };
}

TEST_F(FileSystemJsObject_ReadFromFileTest, NoFile)
{
  bool isOnLineCalled = false;
  mockFileSystem->success = false;
  readFromFile([&isOnLineCalled](const std::string& line)
    {
      isOnLineCalled = true;
    });
  EXPECT_FALSE(isOnLineCalled);
}

TEST_F(FileSystemJsObject_ReadFromFileTest, Lines)
{
  readFromFile_Lines({}, {""});
  readFromFile_Lines({"\n"}, {""});
  readFromFile_Lines({"\n\r"}, {""});
  readFromFile_Lines({"\n\r\n"}, {""});

  readFromFile_Lines({"line"}, {"line"});

  readFromFile_Lines(
    "first\n"
    "second\r\n"
    "third\r\n"
    "\r\n"
    "\n"
    "last",
    {"first", "second", "third", "last"});

  readFromFile_Lines(
    "first\n"
    "second\r\n"
    "third\n",
    {"first", "second", "third"});

  readFromFile_Lines(
    "first\n"
    "second\r\n"
    "third\r\n",
    {"first", "second", "third"});

  readFromFile_Lines(
    "first\n"
    "second\r\n"
    "third\r\n"
    "\r\n"
    "\n",
    {"first", "second", "third"});

  readFromFile_Lines(
    "\n"
    "first\n"
    "second\r\n"
    "third\r\n",
    {"first", "second", "third"});
}

TEST_F(FileSystemJsObject_ReadFromFileTest, ProcessLineThrowsException)
{
  std::string content = "1\n2\n3";
  mockFileSystem->contentToRead.assign(content.begin(), content.end());

  std::vector<std::string> readLines;
  auto& jsEngine = GetJsEngine();
  std::string error;
  jsEngine.SetEventCallback("onLine", [&readLines](JsValueList&& /*line*/jsArgs)
  {
    ASSERT_EQ(1u, jsArgs.size());
    EXPECT_TRUE(jsArgs[0].IsString());
    readLines.emplace_back(jsArgs[0].AsString());
  });
  jsEngine.SetEventCallback("onDone", [&error](JsValueList&& /*error*/jsArgs)
  {
    ASSERT_EQ(1u, jsArgs.size());
    EXPECT_TRUE(jsArgs[0].IsString());
    error = jsArgs[0].AsString();
  });
  jsEngine.Evaluate(R"js(
let onLineCounter = 0;
_fileSystem.readFromFile("foo",
  (line) =>
  {
    if (onLineCounter++ == 2)
    {
      throw new Error("my-error");
    }
    _triggerEvent("onLine", line);
  },
  () => _triggerEvent("onDone"),
  (error) => _triggerEvent("onDone", error));
)js");
  EXPECT_EQ(2u, readLines.size());
  EXPECT_EQ("Error: my-error at undefined:8", error);
}

TEST_F(FileSystemJsObjectTest, MoveNonExistingFile)
{
  mockFileSystem->success = false;
  auto& jsEngine = GetJsEngine();
  const std::vector<std::string> jsFiles = {"compat.js", "io.js"};
  for (int i = 0; !jsSources[i].empty(); i += 2)
  {
    if (jsFiles.end() != std::find(jsFiles.begin(), jsFiles.end(), jsSources[i]))
    {
      jsEngine.Evaluate(jsSources[i + 1], jsSources[i]);
    }
  }
  jsEngine.Evaluate(R"js(
    let hasError = false;
    let isNextHandlerCalled = false;
    require("io").IO.renameFile("foo", "bar")
      .catch((e) => {hasError = e})
      .then(() => isNextHandlerCalled = true);
  )js");
  EXPECT_EQ("Unable to move foo to bar", jsEngine.Evaluate("hasError").AsString());
  EXPECT_TRUE(jsEngine.Evaluate("isNextHandlerCalled").AsBool());
}
