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

#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/LogSystem.h>
#include <sstream>

#include "ConsoleJsObject.h"
#include "JsContext.h"
#include "Utils.h"

namespace
{
  v8::Handle<v8::Value> DoLog(AdblockPlus::LogSystem::LogLevel logLevel,
      const v8::Arguments& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsContext context(*jsEngine);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    std::stringstream message;
    for (size_t i = 0; i < converted.size(); i++)
    {
      if (i > 0)
        message << " ";
      message << converted[i].AsString();
    }

    std::stringstream source;
    v8::Local<v8::StackFrame> frame = v8::StackTrace::CurrentStackTrace(1)->GetFrame(0);
    source << AdblockPlus::Utils::FromV8String(frame->GetScriptName());
    source << ":" << frame->GetLineNumber();

    AdblockPlus::LogSystemPtr callback = jsEngine->GetLogSystem();
    (*callback)(logLevel, message.str(), source.str());
    return v8::Undefined();
  }

  v8::Handle<v8::Value> LogCallback(const v8::Arguments& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_LOG, arguments);
  }

  v8::Handle<v8::Value> DebugCallback(const v8::Arguments& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_LOG, arguments);
  }

  v8::Handle<v8::Value> InfoCallback(const v8::Arguments& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_INFO, arguments);
  }

  v8::Handle<v8::Value> WarnCallback(const v8::Arguments& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_WARN, arguments);
  }

  v8::Handle<v8::Value> ErrorCallback(const v8::Arguments& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_ERROR, arguments);
  }

  v8::Handle<v8::Value> TraceCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsContext context(*jsEngine);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    std::stringstream traceback;
    v8::Local<v8::StackTrace> frames = v8::StackTrace::CurrentStackTrace(100);
    for (int i = 0, l = frames->GetFrameCount(); i < l; i++)
    {
      v8::Local<v8::StackFrame> frame = frames->GetFrame(i);
      traceback << (i + 1) << ": ";
      std::string name = AdblockPlus::Utils::FromV8String(frame->GetFunctionName());
      if (name.size())
        traceback << name;
      else
        traceback << "/* anonymous */";
      traceback << "() at ";
      traceback << AdblockPlus::Utils::FromV8String(frame->GetScriptName());
      traceback << ":" << frame->GetLineNumber();
      traceback << std::endl;
    }

    AdblockPlus::LogSystemPtr callback = jsEngine->GetLogSystem();
    (*callback)(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, traceback.str(), "");
    return v8::Undefined();
  }
}

AdblockPlus::JsValue& AdblockPlus::ConsoleJsObject::Setup(
    AdblockPlus::JsEngine& jsEngine, AdblockPlus::JsValue& obj)
{
  obj.SetProperty("log", jsEngine.NewCallback(::LogCallback));
  obj.SetProperty("debug", jsEngine.NewCallback(::DebugCallback));
  obj.SetProperty("info", jsEngine.NewCallback(::InfoCallback));
  obj.SetProperty("warn", jsEngine.NewCallback(::WarnCallback));
  obj.SetProperty("error", jsEngine.NewCallback(::ErrorCallback));
  obj.SetProperty("trace", jsEngine.NewCallback(::TraceCallback));
  return obj;
}
