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

#include <vector>
#include <stdexcept>

#include <AdblockPlus/JsValue.h>

#include "AppInfoJsObject.h"
#include "ConsoleJsObject.h"
#include "FileSystemJsObject.h"
#include "GlobalJsObject.h"
#include "ConsoleJsObject.h"
#include "WebRequestJsObject.h"
#include "Thread.h"
#include "Utils.h"

using namespace AdblockPlus;

namespace
{
  class TimeoutThread : public Thread
  {
  public:
    TimeoutThread(JsValueList& arguments)
    {
      if (arguments.size() < 2)
        throw std::runtime_error("setTimeout requires at least 2 parameters");

      if (!arguments[0]->IsFunction())
        throw std::runtime_error(
          "First argument to setTimeout must be a function");

      function = arguments[0];
      delay = arguments[1]->AsInt();
      for (size_t i = 2; i < arguments.size(); i++)
        functionArguments.push_back(arguments[i]);
    }

    void Run()
    {
      Sleep(delay);

      function->Call(functionArguments);
    }

  private:
    JsValuePtr function;
    int delay;
    JsValueList functionArguments;
  };

  v8::Handle<v8::Value> SetTimeoutCallback(const v8::Arguments& arguments)
  {
    TimeoutThread* timeoutThread;
    try
    {
      AdblockPlus::JsValueList converted =
          AdblockPlus::JsEngine::FromArguments(arguments)
          ->ConvertArguments(arguments);
      timeoutThread = new TimeoutThread(converted);
    }
    catch (const std::exception& e)
    {
      v8::Isolate* isolate = arguments.GetIsolate();
      return v8::ThrowException(Utils::ToV8String(isolate, e.what()));
    }
    timeoutThread->Start();

    // We should actually return the timer ID here, which could be
    // used via clearTimeout(). But since we don't seem to need
    // clearTimeout(), we can save that for later.
    return v8::Undefined();
  }

  v8::Handle<v8::Value> TriggerEventCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);
    if (converted.size() < 1)
    {
      v8::Isolate* isolate = arguments.GetIsolate();
      return v8::ThrowException(Utils::ToV8String(isolate,
      "_triggerEvent expects at least one parameter"));
    }
    std::string eventName = converted.front()->AsString();
    converted.erase(converted.begin());
    jsEngine->TriggerEvent(eventName, converted);
    return v8::Undefined();
  }
}

JsValuePtr GlobalJsObject::Setup(JsEnginePtr jsEngine, const AppInfo& appInfo,
    JsValuePtr obj)
{
  obj->SetProperty("setTimeout", jsEngine->NewCallback(::SetTimeoutCallback));
  obj->SetProperty("_triggerEvent", jsEngine->NewCallback(::TriggerEventCallback));
  obj->SetProperty("_fileSystem",
      FileSystemJsObject::Setup(jsEngine, jsEngine->NewObject()));
  obj->SetProperty("_webRequest",
      WebRequestJsObject::Setup(jsEngine, jsEngine->NewObject()));
  obj->SetProperty("console",
      ConsoleJsObject::Setup(jsEngine, jsEngine->NewObject()));
  obj->SetProperty("_appInfo",
      AppInfoJsObject::Setup(jsEngine, appInfo, jsEngine->NewObject()));
  return obj;
}
