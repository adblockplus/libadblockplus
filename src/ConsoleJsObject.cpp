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

#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/ErrorCallback.h>
#include <sstream>

#include "ConsoleJsObject.h"

namespace
{
  v8::Handle<v8::Value> ErrorCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsEngine::Context context(jsEngine);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    std::stringstream message;
    for (size_t i = 0; i < converted.size(); i++)
      message << converted[i]->AsString();

    AdblockPlus::ErrorCallbackPtr callback = jsEngine->GetErrorCallback();
    (*callback)(message.str());
    return v8::Undefined();
  }

  v8::Handle<v8::Value> TraceCallback(const v8::Arguments& arguments)
  {
    return v8::Undefined();
  }
}

AdblockPlus::JsValuePtr AdblockPlus::ConsoleJsObject::Setup(
    AdblockPlus::JsEnginePtr jsEngine, AdblockPlus::JsValuePtr obj)
{
  obj->SetProperty("error", jsEngine->NewCallback(::ErrorCallback));
  obj->SetProperty("trace", jsEngine->NewCallback(::TraceCallback));
  return obj;
}
