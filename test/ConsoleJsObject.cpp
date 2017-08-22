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

#include "BaseJsTest.h"

using namespace AdblockPlus;

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

  class ConsoleJsObjectTest : public BaseJsTest
  {
  protected:
    MockLogSystem* mockLogSystem;

    void SetUp() override
    {
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(mockLogSystem = new MockLogSystem());
      platform.reset(new Platform(std::move(platformParams)));
    }
  };
}

TEST_F(ConsoleJsObjectTest, ConsoleLogCall)
{
  GetJsEngine().Evaluate("\n\nconsole.log('foo', 'bar');\n\n", "eval");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_LOG, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ("eval:3", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleDebugCall)
{
  GetJsEngine().Evaluate("console.debug('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_LOG, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleInfoCall)
{
  GetJsEngine().Evaluate("console.info('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_INFO, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleWarnCall)
{
  GetJsEngine().Evaluate("console.warn('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleErrorCall)
{
  GetJsEngine().Evaluate("console.error('foo', 'bar')");
  ASSERT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_ERROR, mockLogSystem->lastLogLevel);
  ASSERT_EQ("foo bar", mockLogSystem->lastMessage);
  ASSERT_EQ(":1", mockLogSystem->lastSource);
}

TEST_F(ConsoleJsObjectTest, ConsoleTraceCall)
{
  GetJsEngine().Evaluate("\n\
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
