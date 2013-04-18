#include <AdblockPlus.h>
#include <gtest/gtest.h>

class MockErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  std::string lastMessage;

  void operator()(const std::string& message)
  {
    lastMessage = message;
  }
};

TEST(ConsoleJsObjectTest, ErrorInvokesErrorCallback)
{
  AdblockPlus::JsEngine jsEngine;
  MockErrorCallback* errorCallback = new MockErrorCallback();
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(errorCallback));
  jsEngine.Evaluate("console.error('foo')");
  ASSERT_EQ("foo", errorCallback->lastMessage);
}

TEST(ConsoleJsObjectTest, ErrorWithMultipleArguments)
{
  AdblockPlus::JsEngine jsEngine;
  MockErrorCallback* errorCallback = new MockErrorCallback();
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(errorCallback));
  jsEngine.Evaluate("console.error('foo', 'bar')");
  ASSERT_EQ("foobar", errorCallback->lastMessage);
}

TEST(ConsoleJsObjectTest, TraceDoesNothing)
{
  AdblockPlus::JsEngine jsEngine;
  jsEngine.Evaluate("console.trace()");
}
