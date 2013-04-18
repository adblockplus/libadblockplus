#include <AdblockPlus.h>
#include <gtest/gtest.h>

#include "../src/Thread.h"
#include "../src/Utils.h"

namespace
{
  class MockFileSystem : public AdblockPlus::FileSystem
  {
  public:
    bool success;
    std::string contentToRead;
    std::string lastWrittenPath;
    std::string lastWrittenContent;
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

    std::tr1::shared_ptr<std::istream> Read(const std::string& path) const
    {
      if (!success)
        throw std::runtime_error("Unable to read " + path);
      std::stringstream* const stream = new std::stringstream;
      *stream << contentToRead;
      return std::tr1::shared_ptr<std::istream>(stream);
    }

    void Write(const std::string& path, std::tr1::shared_ptr<std::ostream> data)
    {
      if (!success)
        throw std::runtime_error("Unable to write to " + path);
      lastWrittenPath = path;
      lastWrittenContent = AdblockPlus::Utils::Slurp(*data);
    }

    void Move(const std::string& fromPath, const std::string& toPath)
    {
      if (!success)
        throw std::runtime_error("Unable to move " + fromPath + " to "
                                 + toPath);
      movedFrom = fromPath;
      movedTo = toPath;
    }

    void Remove(const std::string& path)
    {
      if (!success)
        throw std::runtime_error("Unable to remove " + path);
      removedPath = path;
    }

    StatResult Stat(const std::string& path) const
    {
      if (!success)
        throw std::runtime_error("Unable to stat " + path);
      statPath = path;
      StatResult result;
      result.exists = statExists;
      result.isDirectory = statIsDirectory;
      result.isFile = statIsFile;
      result.lastModified = statLastModified;
      return result;
    }
  };

  void ReadFile(AdblockPlus::JsEngine& jsEngine, std::string& content,
                std::string& error)
  {
    jsEngine.Evaluate("_fileSystem.read('', function(r) {result = r})");
    AdblockPlus::Sleep(10);
    content = jsEngine.Evaluate("result.content")->AsString();
    error = jsEngine.Evaluate("result.error")->AsString();
  }
}

TEST(FileSystemJsObjectTest, Read)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->contentToRead = "foo";
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  std::string content;
  std::string error;
  ReadFile(jsEngine, content, error);
  ASSERT_EQ("foo", content);
  ASSERT_EQ("", error);
}

TEST(FileSystemJsObjectTest, ReadIllegalArguments)
{
  AdblockPlus::JsEngine jsEngine;
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.read()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.read('', '')"));
}

TEST(FileSystemJsObjectTest, ReadError)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->success = false;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  std::string content;
  std::string error;
  ReadFile(jsEngine, content, error);
  ASSERT_NE("", error);
  ASSERT_EQ("", content);
}

TEST(FileSystemJsObjectTest, Write)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_EQ("foo", fileSystem->lastWrittenPath);
  ASSERT_EQ("bar", fileSystem->lastWrittenContent);
  ASSERT_EQ("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, WriteIllegalArguments)
{
  AdblockPlus::JsEngine jsEngine;
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.write()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.write('', '', '')"));
}

TEST(FileSystemJsObjectTest, WriteError)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->success = false;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.write('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_NE("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, Move)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_EQ("foo", fileSystem->movedFrom);
  ASSERT_EQ("bar", fileSystem->movedTo);
  ASSERT_EQ("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, MoveIllegalArguments)
{
  AdblockPlus::JsEngine jsEngine;
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.move()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.move('', '', '')"));
}

TEST(FileSystemJsObjectTest, MoveError)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->success = false;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.move('foo', 'bar', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_NE("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, Remove)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_EQ("foo", fileSystem->removedPath);
  ASSERT_EQ("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, RemoveIllegalArguments)
{
  AdblockPlus::JsEngine jsEngine;
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.remove()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.remove('', '')"));
}

TEST(FileSystemJsObjectTest, RemoveError)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->success = false;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.remove('foo', function(e) {error = e})");
  AdblockPlus::Sleep(10);
  ASSERT_NE("", jsEngine.Evaluate("error")->AsString());
}

TEST(FileSystemJsObjectTest, Stat)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->statExists = true;
  fileSystem->statIsDirectory= false;
  fileSystem->statIsFile = true;
  fileSystem->statLastModified = 1337;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  AdblockPlus::Sleep(10);
  ASSERT_EQ("foo", fileSystem->statPath);
  ASSERT_EQ("", jsEngine.Evaluate("result.error")->AsString());
  ASSERT_TRUE(jsEngine.Evaluate("result.exists")->AsBool());
  ASSERT_FALSE(jsEngine.Evaluate("result.isDirectory")->AsBool());
  ASSERT_TRUE(jsEngine.Evaluate("result.isFile")->AsBool());
  ASSERT_EQ(1337, jsEngine.Evaluate("result.lastModified")->AsInt());
}

TEST(FileSystemJsObjectTest, StatIllegalArguments)
{
  AdblockPlus::JsEngine jsEngine;
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.stat()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_fileSystem.stat('', '')"));
}

TEST(FileSystemJsObjectTest, StatError)
{
  AdblockPlus::JsEngine jsEngine;
  MockFileSystem* fileSystem = new MockFileSystem();
  fileSystem->success = false;
  jsEngine.SetFileSystem(AdblockPlus::FileSystemPtr(fileSystem));;
  jsEngine.Evaluate("_fileSystem.stat('foo', function(r) {result = r})");
  AdblockPlus::Sleep(10);
  ASSERT_NE("", jsEngine.Evaluate("result.error")->AsString());
}
