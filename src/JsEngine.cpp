/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#include <AdblockPlus.h>

#include "GlobalJsObject.h"
#include "JsContext.h"
#include "JsError.h"
#include "Utils.h"

namespace
{
  v8::Handle<v8::Script> CompileScript(v8::Isolate* isolate,
    const std::string& source, const std::string& filename)
  {
    using AdblockPlus::Utils::ToV8String;
    const v8::Handle<v8::String> v8Source = ToV8String(isolate, source);
    if (filename.length())
    {
      const v8::Handle<v8::String> v8Filename = ToV8String(isolate, filename);
      return v8::Script::Compile(v8Source, v8Filename);
    }
    else
      return v8::Script::Compile(v8Source);
  }

  void CheckTryCatch(const v8::TryCatch& tryCatch)
  {
    if (tryCatch.HasCaught())
      throw AdblockPlus::JsError(tryCatch.Exception(), tryCatch.Message());
  }

  class V8Initializer
  {
    V8Initializer()
    {
      v8::V8::Initialize();
    }

    ~V8Initializer()
    {
      v8::V8::Dispose();
    }
  public:
    static void Init()
    {
      // it's threadsafe since C++11 and it will be instantiated only once and
      // destroyed at the application exit
      static V8Initializer initializer;
    }
  };
}

AdblockPlus::JsEngine::JsEngine()
  : isolate(v8::Isolate::GetCurrent())
{
}

AdblockPlus::JsEnginePtr AdblockPlus::JsEngine::New(const AppInfo& appInfo)
{
  V8Initializer::Init();
  JsEnginePtr result(new JsEngine());

  const v8::Locker locker(result->isolate);
  const v8::HandleScope handleScope;

  result->context.reset(new v8::Persistent<v8::Context>(result->isolate,
    v8::Context::New(result->isolate)));
  v8::Local<v8::Object> globalContext = v8::Local<v8::Context>::New(
    result->isolate, *result->context)->Global();
  result->globalJsObject = JsValuePtr(new JsValue(result, globalContext));
  AdblockPlus::GlobalJsObject::Setup(result, appInfo, result->globalJsObject);
  return result;
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::Evaluate(const std::string& source,
    const std::string& filename)
{
  const JsContext context(shared_from_this());
  const v8::TryCatch tryCatch;
  const v8::Handle<v8::Script> script = CompileScript(isolate, source,
    filename);
  CheckTryCatch(tryCatch);
  v8::Local<v8::Value> result = script->Run();
  CheckTryCatch(tryCatch);
  return JsValuePtr(new JsValue(shared_from_this(), result));
}

void AdblockPlus::JsEngine::SetEventCallback(const std::string& eventName,
    AdblockPlus::JsEngine::EventCallback callback)
{
  eventCallbacks[eventName] = callback;
}

void AdblockPlus::JsEngine::RemoveEventCallback(const std::string& eventName)
{
  eventCallbacks.erase(eventName);
}

void AdblockPlus::JsEngine::TriggerEvent(const std::string& eventName, AdblockPlus::JsValueList& params)
{
  EventMap::iterator it = eventCallbacks.find(eventName);
  if (it != eventCallbacks.end())
    it->second(params);
}

void AdblockPlus::JsEngine::Gc()
{
  while (!v8::V8::IdleNotification());
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(const std::string& val)
{
  const JsContext context(shared_from_this());
  return JsValuePtr(new JsValue(shared_from_this(),
    Utils::ToV8String(isolate, val)));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(int64_t val)
{
  const JsContext context(shared_from_this());
  return JsValuePtr(new JsValue(shared_from_this(),
    v8::Number::New(isolate, val)));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(bool val)
{
  const JsContext context(shared_from_this());
  return JsValuePtr(new JsValue(shared_from_this(), v8::Boolean::New(val)));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewObject()
{
  const JsContext context(shared_from_this());
  return JsValuePtr(new JsValue(shared_from_this(), v8::Object::New()));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewCallback(
    v8::InvocationCallback callback)
{
  const JsContext context(shared_from_this());

  // Note: we are leaking this weak pointer, no obvious way to destroy it when
  // it's no longer used
  std::weak_ptr<JsEngine>* data =
      new std::weak_ptr<JsEngine>(shared_from_this());
  v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(callback,
      v8::External::New(data));
  return JsValuePtr(new JsValue(shared_from_this(), templ->GetFunction()));
}

AdblockPlus::JsEnginePtr AdblockPlus::JsEngine::FromArguments(const v8::Arguments& arguments)
{
  const v8::Local<const v8::External> external =
      v8::Local<const v8::External>::Cast(arguments.Data());
  std::weak_ptr<JsEngine>* data =
      static_cast<std::weak_ptr<JsEngine>*>(external->Value());
  JsEnginePtr result = data->lock();
  if (!result)
    throw std::runtime_error("Oops, our JsEngine is gone, how did that happen?");
  return result;
}

AdblockPlus::JsValueList AdblockPlus::JsEngine::ConvertArguments(const v8::Arguments& arguments)
{
  const JsContext context(shared_from_this());
  JsValueList list;
  for (int i = 0; i < arguments.Length(); i++)
    list.push_back(JsValuePtr(new JsValue(shared_from_this(), arguments[i])));
  return list;
}

AdblockPlus::FileSystemPtr AdblockPlus::JsEngine::GetFileSystem()
{
  if (!fileSystem)
    fileSystem.reset(new DefaultFileSystem());
  return fileSystem;
}

void AdblockPlus::JsEngine::SetFileSystem(AdblockPlus::FileSystemPtr val)
{
  if (!val)
    throw std::runtime_error("FileSystem cannot be null");

  fileSystem = val;
}

AdblockPlus::WebRequestPtr AdblockPlus::JsEngine::GetWebRequest()
{
  if (!webRequest)
    webRequest.reset(new DefaultWebRequest());
  return webRequest;
}

void AdblockPlus::JsEngine::SetWebRequest(AdblockPlus::WebRequestPtr val)
{
  if (!val)
    throw std::runtime_error("WebRequest cannot be null");

  webRequest = val;
}

AdblockPlus::LogSystemPtr AdblockPlus::JsEngine::GetLogSystem()
{
  if (!logSystem)
    logSystem.reset(new DefaultLogSystem());
  return logSystem;
}

void AdblockPlus::JsEngine::SetLogSystem(AdblockPlus::LogSystemPtr val)
{
  if (!val)
    throw std::runtime_error("LogSystem cannot be null");

  logSystem = val;
}


void AdblockPlus::JsEngine::SetGlobalProperty(const std::string& name, 
                                              AdblockPlus::JsValuePtr value)
{
  if (!globalJsObject)
    throw std::runtime_error("Global object cannot be null");

  globalJsObject->SetProperty(name, value);
}
