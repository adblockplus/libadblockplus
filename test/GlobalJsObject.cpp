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

#include "../src/DefaultTimer.h"
#include "../src/Thread.h"
#include "BaseJsTest.h"

namespace
{
  class GlobalJsObjectTest : public BaseJsTest
  {
  protected:
    void SetUp() override
    {
      ThrowingPlatformCreationParameters params;
      params.timer.reset(new AdblockPlus::DefaultTimer());
      platform.reset(new AdblockPlus::Platform(std::move(params)));
    }
  };
}

TEST_F(GlobalJsObjectTest, SetTimeout)
{
  GetJsEngine().Evaluate("let foo; setTimeout(function() {foo = 'bar';}, 100)");
  ASSERT_TRUE(GetJsEngine().Evaluate("foo").IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ("bar", GetJsEngine().Evaluate("foo").AsString());
}

TEST_F(GlobalJsObjectTest, SetTimeoutWithArgs)
{
  GetJsEngine().Evaluate("let foo; setTimeout(function(s) {foo = s;}, 100, 'foobar')");
  ASSERT_TRUE(GetJsEngine().Evaluate("foo").IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ("foobar", GetJsEngine().Evaluate("foo").AsString());
}

TEST_F(GlobalJsObjectTest, SetTimeoutWithInvalidArgs)
{
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("setTimeout()"));
  ASSERT_ANY_THROW(GetJsEngine().Evaluate("setTimeout('', 1)"));
}

TEST_F(GlobalJsObjectTest, SetMultipleTimeouts)
{
  GetJsEngine().Evaluate("let foo = []");
  GetJsEngine().Evaluate("setTimeout(function(s) {foo.push('1');}, 100)");
  GetJsEngine().Evaluate("setTimeout(function(s) {foo.push('2');}, 150)");
  AdblockPlus::Sleep(200);
  ASSERT_EQ("1,2", GetJsEngine().Evaluate("foo").AsString());
}