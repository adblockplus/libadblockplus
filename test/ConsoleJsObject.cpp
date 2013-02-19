#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include <istream>

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

class ThrowingErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  void operator()(const std::string& message)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

TEST(ConsoleJsObjectTest, ErrorInvokesErrorCallback)
{
  ThrowingFileReader fileReader;
  MockErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Evaluate("console.error('foo')");
  ASSERT_EQ("foo", errorCallback.lastMessage);
}

TEST(ConsoleJsObjectTest, TraceDoesNothing)
{
  ThrowingFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Evaluate("console.trace()");
}
