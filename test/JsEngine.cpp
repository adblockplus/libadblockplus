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

#include <stdexcept>
#include "BaseJsTest.h"

namespace
{
  class JsEngineTest : public BaseJsTest
  {
  };
}

TEST_F(JsEngineTest, Evaluate)
{
  jsEngine->Evaluate("function hello() { return 'Hello'; }");
  AdblockPlus::JsValuePtr result = jsEngine->Evaluate("hello()");
  ASSERT_TRUE(result->IsString());
  ASSERT_EQ("Hello", result->AsString());
}

TEST_F(JsEngineTest, RuntimeExceptionIsThrown)
{
  ASSERT_THROW(jsEngine->Evaluate("doesnotexist()"), std::runtime_error);
}

TEST_F(JsEngineTest, CompileTimeExceptionIsThrown)
{
  ASSERT_THROW(jsEngine->Evaluate("'foo'bar'"), std::runtime_error);
}

TEST_F(JsEngineTest, ValueCreation)
{
  AdblockPlus::JsValuePtr value;

  value = jsEngine->NewValue("foo");
  ASSERT_TRUE(value->IsString());
  ASSERT_EQ("foo", value->AsString());

  value = jsEngine->NewValue(12345678901234);
  ASSERT_TRUE(value->IsNumber());
  ASSERT_EQ(12345678901234, value->AsInt());

  value = jsEngine->NewValue(true);
  ASSERT_TRUE(value->IsBool());
  ASSERT_TRUE(value->AsBool());

  value = jsEngine->NewObject();
  ASSERT_TRUE(value->IsObject());
  ASSERT_EQ(0u, value->GetOwnPropertyNames().size());
}

TEST_F(JsEngineTest, EventCallbacks)
{
  bool callbackCalled = false;
  AdblockPlus::JsValueList callbackParams;
  auto Callback = [&callbackCalled, & callbackParams](
    const AdblockPlus::JsValueList& params)
  {
    callbackCalled = true;
    callbackParams = params;
  };

  // Trigger event without a callback
  callbackCalled = false;
  jsEngine->Evaluate("_triggerEvent('foobar')");
  ASSERT_FALSE(callbackCalled);

  // Set callback
  jsEngine->SetEventCallback("foobar", Callback);
  callbackCalled = false;
  jsEngine->Evaluate("_triggerEvent('foobar', 1, 'x', true)");
  ASSERT_TRUE(callbackCalled);
  ASSERT_EQ(callbackParams.size(), 3u);
  ASSERT_EQ(callbackParams[0]->AsInt(), 1);
  ASSERT_EQ(callbackParams[1]->AsString(), "x");
  ASSERT_TRUE(callbackParams[2]->AsBool());

  // Trigger a different event
  callbackCalled = false;
  jsEngine->Evaluate("_triggerEvent('barfoo')");
  ASSERT_FALSE(callbackCalled);

  // Remove callback
  jsEngine->RemoveEventCallback("foobar");
  callbackCalled = false;
  jsEngine->Evaluate("_triggerEvent('foobar')");
  ASSERT_FALSE(callbackCalled);
}

TEST(NewJsEngineTest, CallbackGetSet)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());

  ASSERT_TRUE(jsEngine->GetLogSystem());
  ASSERT_ANY_THROW(jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr()));
  AdblockPlus::LogSystemPtr logSystem(new AdblockPlus::DefaultLogSystem());
  jsEngine->SetLogSystem(logSystem);
  ASSERT_EQ(logSystem, jsEngine->GetLogSystem());

  ASSERT_TRUE(jsEngine->GetFileSystem());
  ASSERT_ANY_THROW(jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr()));
  AdblockPlus::FileSystemPtr fileSystem(new AdblockPlus::DefaultFileSystem());
  jsEngine->SetFileSystem(fileSystem);
  ASSERT_EQ(fileSystem, jsEngine->GetFileSystem());

  ASSERT_TRUE(jsEngine->GetWebRequest());
  ASSERT_ANY_THROW(jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr()));
  AdblockPlus::WebRequestPtr webRequest(new AdblockPlus::DefaultWebRequest());
  jsEngine->SetWebRequest(webRequest);
  ASSERT_EQ(webRequest, jsEngine->GetWebRequest());
}

TEST(NewJsEngineTest, GlobalPropertyTest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetGlobalProperty("foo", jsEngine->NewValue("bar"));
  AdblockPlus::JsValuePtr foo = jsEngine->Evaluate("foo");
  ASSERT_TRUE(foo->IsString());
  ASSERT_EQ(foo->AsString(), "bar");
}

