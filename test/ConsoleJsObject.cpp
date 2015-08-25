/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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
  class MockLogSystem : public AdblockPlus::LogSystem
  {
  public:
    AdblockPlus::LogSystem::LogLevel lastLogLevel;
    std::string lastMessage;
    std::string lastSource;

    void operator()(AdblockPlus::LogSystem::LogLevel logLevel,
        const std::string& message, const std::string& source)
    {
      lastLogLevel = logLevel;
      lastMessage = message;
      lastSource = source;
    }
  };

  typedef std::shared_ptr<MockLogSystem> MockLogSystemPtr;

  class ConsoleJsObjectTest : public BaseJsTest
  {
  protected:
    MockLogSystemPtr mockLogSystem;

    void SetUp()
    {
      BaseJsTest::SetUp();
      mockLogSystem = MockLogSystemPtr(new MockLogSystem);
      jsEngine->SetLogSystem(mockLogSystem);
    }
  };
}

TEST_F(ConsoleJsObjectTest, ConsoleLogCall)
{
  jsEngine->Evaluate("\n\nconsole.log('foo', 'bar');\n\n", "eval");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_LOG, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ("eval:3", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleDebugCall)
{
  jsEngine->Evaluate("console.debug('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_LOG, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleInfoCall)
{
  jsEngine->Evaluate("console.info('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_INFO, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleWarnCall)
{
  jsEngine->Evaluate("console.warn('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleErrorCall)
{
  jsEngine->Evaluate("console.error('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_ERROR, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleTraceCall)
{
  jsEngine->Evaluate("\n\
    function foo()\n\
    {\n\
      (function() {\n\
        console.trace();\n\
      })();\n\
    }\n\
    foo();", "eval");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, mockLogSystem->lastLogLevel);
  ASSERT_EQ("\
1: /* anonymous */() at eval:5\n\
2: foo() at eval:6\n\
3: /* anonymous */() at eval:8\n", mockLogSystem->lastMessage);
  ASSERT_EQ("", mockLogSystem->lastSource);
}
