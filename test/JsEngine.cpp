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

#include <stdexcept>
#include "BaseJsTest.h"

using namespace AdblockPlus;

namespace
{
  typedef BaseJsTest JsEngineTest;
}

TEST_F(JsEngineTest, Evaluate)
{
  GetJsEngine().Evaluate("function hello() { return 'Hello'; }");
  auto result = GetJsEngine().Evaluate("hello()");
  ASSERT_TRUE(result.IsString());
  ASSERT_EQ("Hello", result.AsString());
}

TEST_F(JsEngineTest, RuntimeExceptionIsThrown)
{
  ASSERT_THROW(GetJsEngine().Evaluate("doesnotexist()"), std::runtime_error);
}

TEST_F(JsEngineTest, CompileTimeExceptionIsThrown)
{
  ASSERT_THROW(GetJsEngine().Evaluate("'foo'bar'"), std::runtime_error);
}

TEST_F(JsEngineTest, ValueCreation)
{
  auto value = GetJsEngine().NewValue("foo");
  ASSERT_TRUE(value.IsString());
  ASSERT_EQ("foo", value.AsString());

  value = GetJsEngine().NewValue(12345678901234);
  ASSERT_TRUE(value.IsNumber());
  ASSERT_EQ(12345678901234, value.AsInt());

  value = GetJsEngine().NewValue(true);
  ASSERT_TRUE(value.IsBool());
  ASSERT_TRUE(value.AsBool());

  value = GetJsEngine().NewObject();
  ASSERT_TRUE(value.IsObject());
  ASSERT_EQ(0u, value.GetOwnPropertyNames().size());
}

namespace {

  bool IsSame(AdblockPlus::JsEngine& jsEngine,
              const AdblockPlus::JsValue& v1, const AdblockPlus::JsValue& v2)
  {
    AdblockPlus::JsValueList params;
    params.push_back(v1);
    params.push_back(v2);
    return jsEngine.Evaluate("(function(a, b) { return a == b })").Call(params).AsBool();
  }

}

TEST_F(JsEngineTest, ValueCopy)
{
  {
    auto value = GetJsEngine().NewValue("foo");
    ASSERT_TRUE(value.IsString());
    ASSERT_EQ("foo", value.AsString());

    AdblockPlus::JsValue value2(value);
    ASSERT_TRUE(value2.IsString());
    ASSERT_EQ("foo", value2.AsString());

    ASSERT_TRUE(IsSame(GetJsEngine(), value, value2));
  }
  {
    auto value = GetJsEngine().NewValue(12345678901234);
    ASSERT_TRUE(value.IsNumber());
    ASSERT_EQ(12345678901234, value.AsInt());

    AdblockPlus::JsValue value2(value);
    ASSERT_TRUE(value2.IsNumber());
    ASSERT_EQ(12345678901234, value2.AsInt());

    ASSERT_TRUE(IsSame(GetJsEngine(), value, value2));
  }
  {
    auto value = GetJsEngine().NewValue(true);
    ASSERT_TRUE(value.IsBool());
    ASSERT_TRUE(value.AsBool());

    AdblockPlus::JsValue value2(value);
    ASSERT_TRUE(value2.IsBool());
    ASSERT_TRUE(value2.AsBool());

    ASSERT_TRUE(IsSame(GetJsEngine(), value, value2));
  }
  {
    auto value = GetJsEngine().NewObject();
    ASSERT_TRUE(value.IsObject());
    ASSERT_EQ(0u, value.GetOwnPropertyNames().size());

    AdblockPlus::JsValue value2(value);
    ASSERT_TRUE(value2.IsObject());
    ASSERT_EQ(0u, value2.GetOwnPropertyNames().size());

    ASSERT_TRUE(IsSame(GetJsEngine(), value, value2));
  }
}

TEST_F(JsEngineTest, EventCallbacks)
{
  bool callbackCalled = false;
  AdblockPlus::JsValueList callbackParams;
  auto Callback = [&callbackCalled, &callbackParams](JsValueList&& params)
  {
    callbackCalled = true;
    callbackParams = move(params);
  };

  // Trigger event without a callback
  callbackCalled = false;
  GetJsEngine().Evaluate("_triggerEvent('foobar')");
  ASSERT_FALSE(callbackCalled);

  // Set callback
  GetJsEngine().SetEventCallback("foobar", Callback);
  callbackCalled = false;
  GetJsEngine().Evaluate("_triggerEvent('foobar', 1, 'x', true)");
  ASSERT_TRUE(callbackCalled);
  ASSERT_EQ(callbackParams.size(), 3u);
  ASSERT_EQ(callbackParams[0].AsInt(), 1);
  ASSERT_EQ(callbackParams[1].AsString(), "x");
  ASSERT_TRUE(callbackParams[2].AsBool());

  // Trigger a different event
  callbackCalled = false;
  GetJsEngine().Evaluate("_triggerEvent('barfoo')");
  ASSERT_FALSE(callbackCalled);

  // Remove callback
  GetJsEngine().RemoveEventCallback("foobar");
  callbackCalled = false;
  GetJsEngine().Evaluate("_triggerEvent('foobar')");
  ASSERT_FALSE(callbackCalled);
}

TEST(NewJsEngineTest, GlobalPropertyTest)
{
  Platform platform{ThrowingPlatformCreationParameters()};
  auto& jsEngine = platform.GetJsEngine();
  jsEngine.SetGlobalProperty("foo", jsEngine.NewValue("bar"));
  auto foo = jsEngine.Evaluate("foo");
  ASSERT_TRUE(foo.IsString());
  ASSERT_EQ(foo.AsString(), "bar");
}

TEST(NewJsEngineTest, MemoryLeak_NoCircularReferences)
{
  Platform platform{ThrowingPlatformCreationParameters()};
  std::weak_ptr<AdblockPlus::JsEngine> weakJsEngine;
  {
    weakJsEngine = JsEngine::New(AppInfo(), platform);
  }
  EXPECT_FALSE(weakJsEngine.lock());
}

#if UINTPTR_MAX == UINT32_MAX // detection of 32-bit platform
static_assert(sizeof(intptr_t) == 4, "It should be 32bit platform");
TEST(NewJsEngineTest, 32bitsOnly_MemoryLeak_NoLeak)
#else
TEST(NewJsEngineTest, DISABLED_32bitsOnly_MemoryLeak_NoLeak)
#endif
{
  Platform platform{ThrowingPlatformCreationParameters()};
  // v8::Isolate by default requires 32MB (depends on platform), so if there is
  // a memory leak than we will run out of memory on 32 bit platform because it
  // will allocate 32000 MB which is less than 2GB where it reaches out of
  // memory. Even on android where it allocates initially 16MB, the test still
  // makes sense.
  for (int i = 0; i < 1000; ++i)
  {
    JsEngine::New(AppInfo(), platform);
  }
}
