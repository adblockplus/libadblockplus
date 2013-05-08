/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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

  typedef std::tr1::shared_ptr<MockErrorCallback> MockErrorCallbackPtr;

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
