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

#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/tr1_functional.h>
#include <AdblockPlus/LogSystem.h>
#include <AdblockPlus/FileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>

#include "tr1_memory.h"
#include "V8ValueHolder.h"

namespace v8
{
  class Arguments;
  class Isolate;
  class Context;
  template <class T> class Handle;
  template <class T> class Persistent;
  typedef Handle<Value>(*InvocationCallback)(const Arguments &args);
}

namespace AdblockPlus
{
  class JsEngine;
  typedef std::tr1::shared_ptr<JsEngine> JsEnginePtr;

  class JsEngine : public std::tr1::enable_shared_from_this<JsEngine>
  {
    friend class JsValue;
    friend class JsContext;

  public:
    typedef std::tr1::function<void()> EventCallback;
    typedef std::map<std::string, EventCallback> EventMap;

    static JsEnginePtr New(const AppInfo& appInfo = AppInfo());
    void SetEventCallback(const std::string& eventName, EventCallback callback);
    void RemoveEventCallback(const std::string& eventName);
    void TriggerEvent(const std::string& eventName);
    JsValuePtr Evaluate(const std::string& source,
        const std::string& filename = "");
    void Gc();
    JsValuePtr NewValue(const std::string& val);
    JsValuePtr NewValue(int64_t val);
    JsValuePtr NewValue(bool val);
    inline JsValuePtr NewValue(const char* val)
    {
      return NewValue(std::string(val));
    }
    inline JsValuePtr NewValue(int val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#ifdef __APPLE__
    inline JsValuePtr NewValue(long val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#endif
    JsValuePtr NewObject();
    JsValuePtr NewCallback(v8::InvocationCallback callback);
    static JsEnginePtr FromArguments(const v8::Arguments& arguments);
    JsValueList ConvertArguments(const v8::Arguments& arguments);

    FileSystemPtr GetFileSystem();
    void SetFileSystem(FileSystemPtr val);
    WebRequestPtr GetWebRequest();
    void SetWebRequest(WebRequestPtr val);
    LogSystemPtr GetLogSystem();
    void SetLogSystem(LogSystemPtr val);

  private:
    JsEngine();

    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    LogSystemPtr logSystem;
    v8::Isolate* isolate;
    V8ValueHolder<v8::Context> context;
    EventMap eventCallbacks;
  };
}

#endif
