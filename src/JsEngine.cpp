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

#include <AdblockPlus.h>
#include "GlobalJsObject.h"
#include "JsContext.h"
#include "JsError.h"
#include "Utils.h"
#include "DefaultTimer.h"

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

using namespace AdblockPlus;

TimerPtr AdblockPlus::CreateDefaultTimer()
{
  return TimerPtr(new DefaultTimer());
}

WebRequestPtr AdblockPlus::CreateDefaultWebRequest()
{
  return WebRequestPtr(new DefaultWebRequest(std::make_shared<DefaultWebRequestSync>()));
}

AdblockPlus::ScopedV8Isolate::ScopedV8Isolate()
{
  V8Initializer::Init();
  isolate = v8::Isolate::New();
}

AdblockPlus::ScopedV8Isolate::~ScopedV8Isolate()
{
  isolate->Dispose();
  isolate = nullptr;
}

JsEngine::JsWeakValuesList::~JsWeakValuesList()
{
  for (auto& value : values)
    value->Dispose();
}

void JsEngine::NotifyLowMemory()
{
  const JsContext context(*this);
  v8::V8::LowMemoryNotification();
}

void JsEngine::ScheduleTimer(const v8::Arguments& arguments)
{
  auto jsEngine = FromArguments(arguments);
  if (arguments.Length() < 2)
    throw std::runtime_error("setTimeout requires at least 2 parameters");

  if (!arguments[0]->IsFunction())
    throw std::runtime_error("First argument to setTimeout must be a function");

  auto jsValueArguments = jsEngine->ConvertArguments(arguments);
  auto timerParamsID = jsEngine->StoreJsValues(jsValueArguments);

  std::weak_ptr<JsEngine> weakJsEngine = jsEngine;
  jsEngine->timer->SetTimer(std::chrono::milliseconds(arguments[1]->IntegerValue()), [weakJsEngine, timerParamsID]
  {
    if (auto jsEngine = weakJsEngine.lock())
      jsEngine->CallTimerTask(timerParamsID);
  });
}

void JsEngine::CallTimerTask(const JsWeakValuesID& timerParamsID)
{
  auto timerParams = TakeJsValues(timerParamsID);
  JsValue callback = std::move(timerParams[0]);

  timerParams.erase(timerParams.begin()); // remove callback placeholder
  timerParams.erase(timerParams.begin()); // remove timeout param
  callback.Call(timerParams);
}

AdblockPlus::JsEngine::JsEngine(TimerPtr timer, WebRequestPtr webRequest)
  : fileSystem(new DefaultFileSystem())
  , logSystem(new DefaultLogSystem())
  , timer(std::move(timer))
  , webRequest(std::move(webRequest))
{
}

AdblockPlus::JsEnginePtr AdblockPlus::JsEngine::New(const AppInfo& appInfo,
  TimerPtr timer, WebRequestPtr webRequest)
{
  JsEnginePtr result(new JsEngine(std::move(timer), std::move(webRequest)));

  const v8::Locker locker(result->GetIsolate());
  const v8::Isolate::Scope isolateScope(result->GetIsolate());
  const v8::HandleScope handleScope(result->GetIsolate());

  result->context.reset(new v8::Persistent<v8::Context>(result->GetIsolate(),
    v8::Context::New(result->GetIsolate())));
  auto global = result->GetGlobalObject();
  AdblockPlus::GlobalJsObject::Setup(*result, appInfo, global);
  return result;
}

AdblockPlus::JsValue AdblockPlus::JsEngine::GetGlobalObject()
{
  JsContext context(*this);
  return JsValue(shared_from_this(), context.GetV8Context()->Global());
}

AdblockPlus::JsValue AdblockPlus::JsEngine::Evaluate(const std::string& source,
    const std::string& filename)
{
  const JsContext context(*this);
  const v8::TryCatch tryCatch;
  const v8::Handle<v8::Script> script = CompileScript(GetIsolate(), source,
    filename);
  CheckTryCatch(tryCatch);
  v8::Local<v8::Value> result = script->Run();
  CheckTryCatch(tryCatch);
  return JsValue(shared_from_this(), result);
}

void AdblockPlus::JsEngine::SetEventCallback(const std::string& eventName,
    const AdblockPlus::JsEngine::EventCallback& callback)
{
  if (!callback)
  {
    RemoveEventCallback(eventName);
    return;
  }
  std::lock_guard<std::mutex> lock(eventCallbacksMutex);
  eventCallbacks[eventName] = callback;
}

void AdblockPlus::JsEngine::RemoveEventCallback(const std::string& eventName)
{
  std::lock_guard<std::mutex> lock(eventCallbacksMutex);
  eventCallbacks.erase(eventName);
}

void AdblockPlus::JsEngine::TriggerEvent(const std::string& eventName, AdblockPlus::JsValueList&& params)
{
  EventCallback callback;
  {
    std::lock_guard<std::mutex> lock(eventCallbacksMutex);
    auto it = eventCallbacks.find(eventName);
    if (it == eventCallbacks.end())
      return;
    callback = it->second;
  }
  callback(move(params));
}

void AdblockPlus::JsEngine::Gc()
{
  while (!v8::V8::IdleNotification());
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(const std::string& val)
{
  const JsContext context(*this);
  return JsValue(shared_from_this(), Utils::ToV8String(GetIsolate(), val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(int64_t val)
{
  const JsContext context(*this);
  return JsValue(shared_from_this(), v8::Number::New(GetIsolate(), val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(bool val)
{
  const JsContext context(*this);
  return JsValue(shared_from_this(), v8::Boolean::New(val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewObject()
{
  const JsContext context(*this);
  return JsValue(shared_from_this(), v8::Object::New());
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewCallback(
    const v8::InvocationCallback& callback)
{
  const JsContext context(*this);

  // Note: we are leaking this weak pointer, no obvious way to destroy it when
  // it's no longer used
  std::weak_ptr<JsEngine>* data =
      new std::weak_ptr<JsEngine>(shared_from_this());
  v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(callback,
      v8::External::New(data));
  return JsValue(shared_from_this(), templ->GetFunction());
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

JsEngine::JsWeakValuesID JsEngine::StoreJsValues(const JsValueList& values)
{
  JsWeakValuesLists::iterator it;
  {
    std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
    it = jsWeakValuesLists.emplace(jsWeakValuesLists.end());
  }
  {
    JsContext context(*this);
    for (const auto& value : values)
    {
      it->values.emplace_back(new v8::Persistent<v8::Value>(GetIsolate(), value.UnwrapValue()));
    }
  }
  JsWeakValuesID retValue;
  retValue.iterator = it;
  return retValue;
}

JsValueList JsEngine::TakeJsValues(const JsWeakValuesID& id)
{
  JsValueList retValue;
  {
    JsContext context(*this);
    for (const auto& v8Value : id.iterator->values)
    {
      retValue.emplace_back(JsValue(shared_from_this(), v8::Local<v8::Value>::New(GetIsolate(), *v8Value)));
    }
  }
  {
    std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
    jsWeakValuesLists.erase(id.iterator);
  }
  return retValue;
}

AdblockPlus::JsValueList AdblockPlus::JsEngine::ConvertArguments(const v8::Arguments& arguments)
{
  const JsContext context(*this);
  JsValueList list;
  for (int i = 0; i < arguments.Length(); i++)
    list.push_back(JsValue(shared_from_this(), arguments[i]));
  return list;
}

AdblockPlus::FileSystemPtr AdblockPlus::JsEngine::GetFileSystem() const
{
  return fileSystem;
}

void AdblockPlus::JsEngine::SetFileSystem(const AdblockPlus::FileSystemPtr& val)
{
  if (!val)
    throw std::runtime_error("FileSystem cannot be null");

  fileSystem = val;
}

void AdblockPlus::JsEngine::SetWebRequest(const AdblockPlus::WebRequestSharedPtr& val)
{
  if (!val)
    throw std::runtime_error("WebRequest cannot be null");

  webRequestLegacy = val;
}

AdblockPlus::LogSystemPtr AdblockPlus::JsEngine::GetLogSystem() const
{
  return logSystem;
}

void AdblockPlus::JsEngine::SetLogSystem(const AdblockPlus::LogSystemPtr& val)
{
  if (!val)
    throw std::runtime_error("LogSystem cannot be null");

  logSystem = val;
}


void AdblockPlus::JsEngine::SetGlobalProperty(const std::string& name,
                                              const AdblockPlus::JsValue& value)
{
  auto global = GetGlobalObject();
  global.SetProperty(name, value);
}
