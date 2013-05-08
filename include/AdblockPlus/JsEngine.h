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

#include <stdexcept>
#include <stdint.h>
#include <string>
#include <v8.h>
#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/ErrorCallback.h>
#include <AdblockPlus/FileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class JsError : public std::runtime_error
  {
  public:
    JsError(const v8::Handle<v8::Value> exception,
            const v8::Handle<v8::Message> message);
  };

  class JsEngine;
  typedef std::tr1::shared_ptr<JsEngine> JsEnginePtr;

  class JsEngine : public std::tr1::enable_shared_from_this<JsEngine>
  {
    friend class JsValue;

  public:
    static JsEnginePtr New(const AppInfo& appInfo = AppInfo());
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
    ErrorCallbackPtr GetErrorCallback();
    void SetErrorCallback(ErrorCallbackPtr val);

    class Context
    {
    public:
      Context(const JsEnginePtr jsEngine);
      virtual inline ~Context() {};

    private:
      const v8::Locker locker;
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope;
    };

  private:
    JsEngine();

    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    ErrorCallbackPtr errorCallback;
    v8::Isolate* isolate;
    v8::Persistent<v8::Context> context;
  };
}

#endif
