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

#include "ConsoleJsObject.h"

#include <sstream>

#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/LogSystem.h>
#include <AdblockPlus/Platform.h>

#include "JsContext.h"
#include "Utils.h"

namespace
{
  void DoLog(AdblockPlus::LogSystem::LogLevel logLevel,
             const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsContext context(jsEngine->GetIsolate(), *jsEngine->GetContext());
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    std::stringstream message;
    for (size_t i = 0; i < converted.size(); i++)
    {
      if (i > 0)
        message << " ";
      message << converted[i].AsString();
    }

    std::stringstream source;
    auto isolate = arguments.GetIsolate();
    v8::Local<v8::StackTrace> stackTrace = v8::StackTrace::CurrentStackTrace(isolate, 1);
    v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(isolate, 0);
    source << AdblockPlus::Utils::FromV8String(isolate, frame->GetScriptName());
    source << ":" << frame->GetLineNumber();

    jsEngine->GetLogSystem()(logLevel, message.str(), source.str());
  }

  void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    return DoLog(AdblockPlus::LogSystem::LOG_LEVEL_LOG, arguments);
  }

  void DebugCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    DoLog(AdblockPlus::LogSystem::LOG_LEVEL_LOG, arguments);
  }

  void InfoCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    DoLog(AdblockPlus::LogSystem::LOG_LEVEL_INFO, arguments);
  }

  void WarnCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    DoLog(AdblockPlus::LogSystem::LOG_LEVEL_WARN, arguments);
  }

  void ErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    DoLog(AdblockPlus::LogSystem::LOG_LEVEL_ERROR, arguments);
  }

  void TraceCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsContext context(jsEngine->GetIsolate(), *jsEngine->GetContext());
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    auto isolate = arguments.GetIsolate();
    std::stringstream traceback;
    v8::Local<v8::StackTrace> stackTrace = v8::StackTrace::CurrentStackTrace(isolate, 100);
    for (int i = 0, l = stackTrace->GetFrameCount(); i < l; i++)
    {
      v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(isolate, i);
      traceback << (i + 1) << ": ";
      std::string name = AdblockPlus::Utils::FromV8String(isolate, frame->GetFunctionName());
      if (name.size())
        traceback << name;
      else
        traceback << "/* anonymous */";
      traceback << "() at ";
      traceback << AdblockPlus::Utils::FromV8String(isolate, frame->GetScriptName());
      traceback << ":" << frame->GetLineNumber();
      traceback << std::endl;
    }

    jsEngine->GetLogSystem()(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, traceback.str(), "");
  }
}

AdblockPlus::JsValue& AdblockPlus::ConsoleJsObject::Setup(AdblockPlus::JsEngine& jsEngine,
                                                          AdblockPlus::JsValue& obj)
{
  obj.SetProperty("log", jsEngine.NewCallback(::LogCallback));
  obj.SetProperty("debug", jsEngine.NewCallback(::DebugCallback));
  obj.SetProperty("info", jsEngine.NewCallback(::InfoCallback));
  obj.SetProperty("warn", jsEngine.NewCallback(::WarnCallback));
  obj.SetProperty("error", jsEngine.NewCallback(::ErrorCallback));
  obj.SetProperty("trace", jsEngine.NewCallback(::TraceCallback));
  return obj;
}
