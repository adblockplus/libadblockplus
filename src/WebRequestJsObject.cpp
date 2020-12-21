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

#include "WebRequestJsObject.h"

#include <map>

#include <AdblockPlus/IWebRequest.h>
#include <AdblockPlus/Platform.h>

#include "JsContext.h"
#include "Utils.h"

using namespace AdblockPlus;

void JsEngine::ScheduleWebRequest(const v8::FunctionCallbackInfo<v8::Value>& arguments)
{
  AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
  AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);
  if (converted.size() != 3u)
    throw std::runtime_error("GET requires exactly 3 arguments");

  auto url = converted[0].AsString();
  if (!url.length())
    throw std::runtime_error("Invalid string passed as first argument to GET");

  AdblockPlus::HeaderList headers;
  {
    const AdblockPlus::JsValue& headersObj = converted[1];
    if (!headersObj.IsObject())
      throw std::runtime_error("Second argument to GET must be an object");

    std::vector<std::string> properties = headersObj.GetOwnPropertyNames();
    for (const auto& header : properties)
    {
      std::string headerValue = headersObj.GetProperty(header).AsString();
      if (header.length() && headerValue.length())
        headers.push_back(std::pair<std::string, std::string>(header, headerValue));
    }
  }

  if (!converted[2].IsFunction())
    throw std::runtime_error("Third argument to GET must be a function");

  auto paramsID = jsEngine->StoreJsValues(converted);
  auto getCallback = [jsEngine, paramsID](const ServerResponse& response) {
    auto webRequestParams = jsEngine->TakeJsValues(paramsID);

    AdblockPlus::JsContext context(jsEngine->GetIsolate(), *jsEngine->GetContext());

    auto resultObject = jsEngine->NewObject();
    resultObject.SetProperty("status", response.status);
    resultObject.SetProperty("responseStatus", response.responseStatus);
    resultObject.SetProperty("responseText", response.responseText);

    auto headersObject = jsEngine->NewObject();
    for (const auto& header : response.responseHeaders)
    {
      headersObject.SetProperty(header.first, header.second);
    }
    resultObject.SetProperty("responseHeaders", headersObject);

    webRequestParams[2].Call(resultObject);
  };
  jsEngine->GetWebRequest().GET(url, headers, getCallback);
}

namespace
{
  void GETCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    try
    {
      AdblockPlus::JsEngine::ScheduleWebRequest(arguments);
    }
    catch (const std::exception& e)
    {
      return AdblockPlus::Utils::ThrowExceptionInJS(arguments.GetIsolate(), e.what());
    }
  }
}

AdblockPlus::JsValue& AdblockPlus::WebRequestJsObject::Setup(AdblockPlus::JsEngine& jsEngine,
                                                             AdblockPlus::JsValue& obj)
{
  obj.SetProperty("GET", jsEngine.NewCallback(::GETCallback));
  return obj;
}
