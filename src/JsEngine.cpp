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

#include <AdblockPlus.h>
#include <assert.h>
#include <libplatform/libplatform.h>

#include "GlobalJsObject.h"
#include "JsContext.h"
#include "JsError.h"
#include "Utils.h"

namespace
{
  v8::MaybeLocal<v8::Script>
  CompileScript(v8::Isolate* isolate, const std::string& source, const std::string& filename)
  {
    using AdblockPlus::Utils::ToV8String;
    auto maybeV8Source = ToV8String(isolate, source);
    if (maybeV8Source.IsEmpty())
      return v8::MaybeLocal<v8::Script>();
    const v8::Local<v8::String> v8Source = maybeV8Source.ToLocalChecked();
    if (filename.length())
    {
      auto maybeV8Filename = ToV8String(isolate, filename);
      if (maybeV8Filename.IsEmpty())
        return v8::MaybeLocal<v8::Script>();
      v8::ScriptOrigin scriptOrigin(maybeV8Filename.ToLocalChecked());
      return v8::Script::Compile(isolate->GetCurrentContext(), v8Source, &scriptOrigin);
    }
    else
      return v8::Script::Compile(isolate->GetCurrentContext(), v8Source);
  }

  class V8Initializer
  {
    V8Initializer() : platform{nullptr}
    {
      std::string cmd = "--use_strict";
      v8::V8::SetFlagsFromString(cmd.c_str(), cmd.length());
      platform = v8::platform::NewDefaultPlatform();
      v8::V8::InitializePlatform(platform.get());
      v8::V8::Initialize();
    }

    ~V8Initializer()
    {
      v8::V8::Dispose();
      v8::V8::ShutdownPlatform();
    }
    std::unique_ptr<v8::Platform> platform;

  public:
    static void Init()
    {
      // it's threadsafe since C++11 and it will be instantiated only once and
      // destroyed at the application exit
      static V8Initializer initializer;
    }
  };

  /**
   * Scope based isolate manager. Creates a new isolate instance on
   * constructing and disposes it on destructing. In addition it initilizes V8.
   */
  class ScopedV8Isolate : public AdblockPlus::IV8IsolateProvider
  {
  public:
    ScopedV8Isolate()
    {
      V8Initializer::Init();
      allocator.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
      v8::Isolate::CreateParams isolateParams;
      isolateParams.array_buffer_allocator = allocator.get();
      isolate = v8::Isolate::New(isolateParams);
    }

    ~ScopedV8Isolate()
    {
      isolate->Dispose();
      isolate = nullptr;
    }

    v8::Isolate* Get() override
    {
      return isolate;
    }

  private:
    ScopedV8Isolate(const ScopedV8Isolate&);
    ScopedV8Isolate& operator=(const ScopedV8Isolate&);

    std::unique_ptr<v8::ArrayBuffer::Allocator> allocator;
    v8::Isolate* isolate;
  };
}

using namespace AdblockPlus;

JsEngine::JsWeakValuesList::~JsWeakValuesList()
{
}

void JsEngine::NotifyLowMemory()
{
  const JsContext context(GetIsolate(), *GetContext());
  GetIsolate()->MemoryPressureNotification(v8::MemoryPressureLevel::kCritical);
}

void JsEngine::ScheduleTimer(const v8::FunctionCallbackInfo<v8::Value>& arguments)
{
  auto jsEngine = FromArguments(arguments);
  if (arguments.Length() < 2)
    throw std::runtime_error("setTimeout requires at least 2 parameters");

  if (!arguments[0]->IsFunction())
    throw std::runtime_error("First argument to setTimeout must be a function");

  auto jsValueArguments = jsEngine->ConvertArguments(arguments);
  auto timerParamsID = jsEngine->StoreJsValues(jsValueArguments);

  int64_t millis =
      CHECKED_TO_VALUE(arguments[1]->IntegerValue(arguments.GetIsolate()->GetCurrentContext()));

  jsEngine->GetTimer().SetTimer(std::chrono::milliseconds(millis), [jsEngine, timerParamsID] {
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

AdblockPlus::JsEngine::JsEngine(const Interfaces& interfaces,
                                std::unique_ptr<IV8IsolateProvider> isolate)
    : timer(interfaces.timer), fileSystem(interfaces.fileSystem), webRequest(interfaces.webRequest),
      logSystem(interfaces.logSystem), resourceReader(interfaces.resourceReader)
#if !defined(MAKE_ISOLATE_IN_JS_VALUE_WEAK)
      ,
      isolate(std::move(isolate))
#endif
{
#if defined(MAKE_ISOLATE_IN_JS_VALUE_WEAK)
  this->isolate = std::shared_ptr<IV8IsolateProvider>(isolate.release());
#endif
}

JsEngine::~JsEngine()
{
  std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
  for (auto* weakValue : registeredWeakValues)
    weakValue->Invalidate();
}

std::unique_ptr<AdblockPlus::JsEngine>
AdblockPlus::JsEngine::New(const AppInfo& appInfo,
                           const Interfaces& interfaces,
                           std::unique_ptr<IV8IsolateProvider> isolate)
{
  if (!isolate)
  {
    isolate.reset(new ScopedV8Isolate());
  }
  std::unique_ptr<AdblockPlus::JsEngine> result(new JsEngine(interfaces, std::move(isolate)));

  const v8::Locker locker(result->GetIsolate());
  const v8::Isolate::Scope isolateScope(result->GetIsolate());
  const v8::HandleScope handleScope(result->GetIsolate());

  result->context =
      v8::Global<v8::Context>(result->GetIsolate(), v8::Context::New(result->GetIsolate()));
  auto global = result->GetGlobalObject();
  AdblockPlus::GlobalJsObject::Setup(*result, appInfo, global);
  return result;
}

AdblockPlus::JsValue AdblockPlus::JsEngine::GetGlobalObject()
{
  JsContext context(GetIsolate(), *GetContext());
  return JsValue(GetIsolateProviderPtr(), GetContext(), context.GetV8Context()->Global());
}

AdblockPlus::JsValue AdblockPlus::JsEngine::Evaluate(const std::string& source,
                                                     const std::string& filename)
{
  auto isolate = GetIsolate();
  const JsContext context(isolate, *GetContext());
  const v8::TryCatch tryCatch(isolate);
  auto script =
      CHECKED_TO_LOCAL_WITH_TRY_CATCH(isolate, CompileScript(isolate, source, filename), tryCatch);
  auto result =
      CHECKED_TO_LOCAL_WITH_TRY_CATCH(isolate, script->Run(isolate->GetCurrentContext()), tryCatch);
  return JsValue(GetIsolateProviderPtr(), GetContext(), result);
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

void AdblockPlus::JsEngine::TriggerEvent(const std::string& eventName,
                                         AdblockPlus::JsValueList&& params)
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
  while (!GetIsolate()->IdleNotificationDeadline(10))
    ;
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(const std::string& val)
{
  auto isolate = GetIsolate();
  const JsContext context(isolate, *GetContext());

  return JsValue(GetIsolateProviderPtr(),
                 GetContext(),
                 CHECKED_TO_LOCAL(isolate, Utils::ToV8String(isolate, val)));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(int64_t val)
{
  const JsContext context(GetIsolate(), *GetContext());
  return JsValue(GetIsolateProviderPtr(), GetContext(), v8::Number::New(GetIsolate(), val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(bool val)
{
  const JsContext context(GetIsolate(), *GetContext());
  return JsValue(GetIsolateProviderPtr(), GetContext(), v8::Boolean::New(GetIsolate(), val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewValue(double val)
{
  const JsContext context(GetIsolate(), *GetContext());
  return JsValue(GetIsolateProviderPtr(), GetContext(), v8::Number::New(GetIsolate(), val));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewObject()
{
  const JsContext context(GetIsolate(), *GetContext());
  return JsValue(GetIsolateProviderPtr(), GetContext(), v8::Object::New(GetIsolate()));
}

JsValue JsEngine::NewArray(const std::vector<std::string>& values)
{
  const JsContext context(GetIsolate(), *GetContext());
  std::vector<v8::Local<v8::Value>> elements;
  elements.reserve(values.size());
  auto isolate = GetIsolate();

  for (const auto& cur : values)
  {
    elements.push_back(CHECKED_TO_LOCAL(isolate, Utils::ToV8String(isolate, cur)));
  }

  return JsValue(GetIsolateProviderPtr(),
                 GetContext(),
                 v8::Array::New(isolate, elements.data(), elements.size()));
}

AdblockPlus::JsValue AdblockPlus::JsEngine::NewCallback(const v8::FunctionCallback& callback)
{
  auto isolate = GetIsolate();
  const JsContext context(isolate, *GetContext());

  // The callback may not outlive us since it lives out of our isolate.
  // It's safe to bind a bare pointer to self.
  v8::Local<v8::FunctionTemplate> templ =
      v8::FunctionTemplate::New(isolate, callback, v8::External::New(isolate, this));
  return JsValue(GetIsolateProviderPtr(),
                 GetContext(),
                 CHECKED_TO_LOCAL(isolate, templ->GetFunction(isolate->GetCurrentContext())));
}

AdblockPlus::JsEngine*
AdblockPlus::JsEngine::FromArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments)
{
  const v8::Local<const v8::External> external =
      v8::Local<const v8::External>::Cast(arguments.Data());
  return static_cast<JsEngine*>(external->Value());
}

JsEngine::JsWeakValuesID JsEngine::StoreJsValues(const JsValueList& values)
{
  JsWeakValuesLists::iterator it;
  {
    std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
    it = jsWeakValuesLists.emplace(jsWeakValuesLists.end());
  }
  {
    JsContext context(GetIsolate(), *GetContext());
    for (const auto& value : values)
    {
      it->values.emplace_back(GetIsolate(), value.UnwrapValue());
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
    JsContext context(GetIsolate(), *GetContext());
    for (const auto& v8Value : id.iterator->values)
    {
      retValue.emplace_back(JsValue(
          GetIsolateProviderPtr(), GetContext(), v8::Local<v8::Value>::New(GetIsolate(), v8Value)));
    }
  }
  {
    std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
    jsWeakValuesLists.erase(id.iterator);
  }
  return retValue;
}

JsValueList JsEngine::GetJsValues(const JsWeakValuesID& id)
{
  JsValueList retValue;
  JsContext context(GetIsolate(), *GetContext());
  for (const auto& v8Value : id.iterator->values)
  {
    retValue.emplace_back(JsValue(
        GetIsolateProviderPtr(), GetContext(), v8::Local<v8::Value>::New(GetIsolate(), v8Value)));
  }
  return retValue;
}

AdblockPlus::JsValueList
AdblockPlus::JsEngine::ConvertArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments)
{
  const JsContext context(GetIsolate(), *GetContext());
  JsValueList list;
  for (int i = 0; i < arguments.Length(); i++)
    list.push_back(JsValue(GetIsolateProviderPtr(), GetContext(), arguments[i]));
  return list;
}

void AdblockPlus::JsEngine::SetGlobalProperty(const std::string& name,
                                              const AdblockPlus::JsValue& value)
{
  const JsContext context(GetIsolate(), *GetContext());
  auto global = GetGlobalObject();
  global.SetProperty(name, value);
}

v8::Global<v8::Context>* JsEngine::GetContext()
{
  return &context;
}

#if defined(MAKE_ISOLATE_IN_JS_VALUE_WEAK)
JsEngine::IV8IsolateProviderWrapper::IV8IsolateProviderWrapper(
    std::weak_ptr<IV8IsolateProvider> weakIsolate)
    : isolate(weakIsolate)
{
}

v8::Isolate* JsEngine::IV8IsolateProviderWrapper::Get()
{
  if (auto locked = isolate.lock())
  {
    return locked->Get();
  }

  return nullptr;
}
#endif

IV8IsolateProviderPtr JsEngine::GetIsolateProviderPtr() const
{
#if defined(MAKE_ISOLATE_IN_JS_VALUE_WEAK)
  return std::make_shared<IV8IsolateProviderWrapper>(isolate);
#else
  return isolate.get();
#endif
}

void JsEngine::RegisterScopedWeakValue(ScopedWeakValues::RegisteredWeakValue* value)
{
  std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
  assert(std::find(registeredWeakValues.begin(), registeredWeakValues.end(), value) ==
         registeredWeakValues.end());
  registeredWeakValues.push_back(value);
}

void JsEngine::UnregisterScopedWeakValue(ScopedWeakValues::RegisteredWeakValue* value)
{
  std::lock_guard<std::mutex> lock(jsWeakValuesListsMutex);
  auto elem = std::find(registeredWeakValues.begin(), registeredWeakValues.end(), value);
  assert(elem != registeredWeakValues.end());
  registeredWeakValues.erase(elem);
}

JsEngine::ScopedWeakValues::ScopedWeakValues(JsEngine* jsEngine, const JsValueList& values)
    : state(std::make_shared<RegisteredWeakValue>(jsEngine, values))
{
}

JsEngine::ScopedWeakValues::~ScopedWeakValues() = default;

JsEngine::ScopedWeakValues::RegisteredWeakValue::RegisteredWeakValue(JsEngine* jsEngine,
                                                                     const JsValueList& values)
    : engine(jsEngine)
{
  engine->RegisterScopedWeakValue(this);
  weakId = engine->StoreJsValues(values);
}

JsEngine::ScopedWeakValues::RegisteredWeakValue::~RegisteredWeakValue()
{
  if (engine)
  {
    engine->TakeJsValues(weakId);
    engine->UnregisterScopedWeakValue(this);
  }
}

JsValueList JsEngine::ScopedWeakValues::RegisteredWeakValue::Values() const
{
  assert(engine != nullptr);
  return engine->GetJsValues(weakId);
}

void JsEngine::ScopedWeakValues::RegisteredWeakValue::Invalidate()
{
  engine = nullptr;
}

JsValueList JsEngine::ScopedWeakValues::Values() const
{
  return state->Values();
}
