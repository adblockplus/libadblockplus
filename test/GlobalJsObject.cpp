/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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
#include "../src/Thread.h"

namespace
{
  class GlobalJsObjectTest : public ::testing::Test
  {
  protected:
    std::unique_ptr<AdblockPlus::Platform> platform;
    AdblockPlus::JsEnginePtr jsEngine;

    void SetUp() override
    {
      ThrowingPlatformCreationParameters params;
      params.timer = AdblockPlus::CreateDefaultTimer();
      platform.reset(new AdblockPlus::Platform(std::move(params)));
      jsEngine = platform->GetJsEngine();
    }
  };
}

TEST_F(GlobalJsObjectTest, SetTimeout)
{
  jsEngine->Evaluate("setTimeout(function() {foo = 'bar';}, 100)");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo").IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ("bar", jsEngine->Evaluate("this.foo").AsString());
}

TEST_F(GlobalJsObjectTest, SetTimeoutWithArgs)
{
  jsEngine->Evaluate("setTimeout(function(s) {foo = s;}, 100, 'foobar')");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo").IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ("foobar", jsEngine->Evaluate("this.foo").AsString());
}

TEST_F(GlobalJsObjectTest, SetTimeoutWithInvalidArgs)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("setTimeout()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("setTimeout('', 1)"));
}

TEST_F(GlobalJsObjectTest, SetMultipleTimeouts)
{
  jsEngine->Evaluate("foo = []");
  jsEngine->Evaluate("setTimeout(function(s) {foo.push('1');}, 100)");
  jsEngine->Evaluate("setTimeout(function(s) {foo.push('2');}, 150)");
  AdblockPlus::Sleep(200);
  ASSERT_EQ("1,2", jsEngine->Evaluate("this.foo").AsString());
}

TEST_F(GlobalJsObjectTest, TimeoutDoesNotKeepJsEngine)
{
  jsEngine->Evaluate("setTimeout(function() {}, 50000)");
  // 1 reference is held by platform and yet one by the test fixture class.
  EXPECT_EQ(2u, jsEngine.use_count());
  AdblockPlus::Sleep(200);
  std::weak_ptr<AdblockPlus::JsEngine> weakJsEngine = jsEngine;
  jsEngine.reset();
  platform.reset();
  EXPECT_FALSE(weakJsEngine.lock());
}
