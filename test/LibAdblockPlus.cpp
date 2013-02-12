#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include <sstream>

class ThrowingFileReader : public AdblockPlus::FileReader
{
public:
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    throw std::runtime_error("Unexpected read of file: " + path);
  }
};

class MockErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  std::string lastMessage;

  void operator()(const std::string& message)
  {
    lastMessage = message;
  }
};

class MockFileReader : public AdblockPlus::FileReader
{
public:
  mutable std::string lastRead;

  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    lastRead = path;
    return std::auto_ptr<std::istream>(new std::stringstream);
  }
};

class ThrowingErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  void operator()(const std::string& message)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

TEST(JsEngineTest, ReportError)
{
  ThrowingFileReader fileReader;
  MockErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Evaluate("LibAdblockPlus.reportError('fail')");
  ASSERT_EQ("fail", errorCallback.lastMessage);
}

TEST(LibAdblockPlusTest, Load)
{
  MockFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Evaluate("LibAdblockPlus.load('foo.js')");
  ASSERT_EQ("foo.js", fileReader.lastRead);
}
