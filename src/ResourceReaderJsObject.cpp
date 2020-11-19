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

#include "ResourceReaderJsObject.h"

#include <AdblockPlus/Platform.h>

#include "JsContext.h"
#include "Utils.h"

using namespace AdblockPlus;

const std::string kMethodName = "readPreloadedFilterList";

namespace ReadPreloadedFilterListCallback
{

  void V8Callback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    try
    {
      AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
      AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

      if (converted.size() != 3u)
        throw std::runtime_error(kMethodName + ": requires exactly 3 arguments");

      auto url = converted[0].AsString();
      if (!url.length())
        throw std::runtime_error(kMethodName + ": Invalid string passed as first");

      if (!converted[1].IsFunction())
        throw std::runtime_error(kMethodName + ": Second argument must be a function");

      if (!converted[2].IsFunction())
        throw std::runtime_error(kMethodName + ": Third argument must be a function");

      try
      {
        jsEngine->GetPlatform().WithResourceReader([url, jsEngine, callback = converted[1]](
                                                       IResourceReader& reader) {
          reader.ReadPreloadedFilterList(
              url, [jsEngine, callback](std::unique_ptr<IPreloadedFilterResponse> response) {
                bool exists = response->exists();
                const JsContext context(jsEngine->GetIsolate(), jsEngine->GetContext());
                auto result = jsEngine->NewObject();
                result.SetProperty("exists", exists);
                if (exists)
                  result.SetProperty("content", response->content(), response->size());
                callback.Call(result);
              });
        });
      }
      catch (const std::exception& e)
      {
        converted[2].Call(jsEngine->NewValue(e.what()));
      }
      catch (...)
      {
        converted[2].Call(jsEngine->NewValue("Unknown error"));
      }
    }
    catch (const std::exception& e)
    {
      return Utils::ThrowExceptionInJS(arguments.GetIsolate(), e.what());
    }
  }
}

AdblockPlus::JsValue& ResourceReaderJsObject::Setup(AdblockPlus::JsEngine& jsEngine,
                                                    AdblockPlus::JsValue& obj)
{
  obj.SetProperty(kMethodName, jsEngine.NewCallback(::ReadPreloadedFilterListCallback::V8Callback));

  return obj;
}
