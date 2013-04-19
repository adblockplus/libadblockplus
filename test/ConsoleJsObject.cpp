#include "BaseJsTest.h"

namespace
{
  class MockErrorCallback : public AdblockPlus::ErrorCallback
  {
  public:
    std::string lastMessage;

    void operator()(const std::string& message)
    {
      lastMessage = message;
    }
  };

  typedef std::shared_ptr<MockErrorCallback> MockErrorCallbackPtr;

  class ConsoleJsObjectTest : public BaseJsTest
  {
  protected:
    MockErrorCallbackPtr mockErrorCallback;

    void SetUp()
    {
      BaseJsTest::SetUp();
      mockErrorCallback = MockErrorCallbackPtr(new MockErrorCallback);
      jsEngine->SetErrorCallback(mockErrorCallback);
    }
  };
}

TEST_F(ConsoleJsObjectTest, ErrorInvokesErrorCallback)
{
  jsEngine->Evaluate("console.error('foo')");
  ASSERT_EQ("foo", mockErrorCallback->lastMessage);
}

TEST_F(ConsoleJsObjectTest, ErrorWithMultipleArguments)
{
  jsEngine->Evaluate("console.error('foo', 'bar')");
  ASSERT_EQ("foobar", mockErrorCallback->lastMessage);
}

TEST_F(ConsoleJsObjectTest, TraceDoesNothing)
{
  jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback));
  jsEngine->Evaluate("console.trace()");
}
