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

#pragma once

#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <stdexcept>
#include <stdint.h>
#include <string>

#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/IFileSystem.h>
#include <AdblockPlus/IResourceReader.h>
#include <AdblockPlus/ITimer.h>
#include <AdblockPlus/IV8IsolateProvider.h>
#include <AdblockPlus/IWebRequest.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/LogSystem.h>
#include <v8.h>

namespace AdblockPlus
{
  class JsEngine;
  /**
   * JavaScript engine used by `IFilterEngine`, wraps v8.
   */
  class JsEngine
  {
    friend class JsValue;
    friend class JsContext;

    struct JsWeakValuesList
    {
      ~JsWeakValuesList();
      std::vector<v8::Global<v8::Value>> values;
    };
    typedef std::list<JsWeakValuesList> JsWeakValuesLists;

    /**
     * An opaque structure representing ID of stored JsValueList.
     */
    class JsWeakValuesID
    {
      friend class JsEngine;
      JsWeakValuesLists::const_iterator iterator;
    };

  public:
    struct Interfaces
    {
      ITimer& timer;
      IFileSystem& fileSystem;
      IWebRequest& webRequest;
      LogSystem& logSystem;
      IResourceReader& resourceReader;
    };

    ~JsEngine();

    enum class WebRequestMethod { kGet, kHead };

    /**
     * Event callback function.
     */
    typedef std::function<void(JsValueList&& params)> EventCallback;

    /**
     * Maps events to callback functions.
     */
    typedef std::map<std::string, EventCallback> EventMap;

    /**
     * Class representing JsValues which are stored in the JsEngine and are owned by the JsEngine
     * and are automatically released when going out of scope.
     */
    class ScopedWeakValues
    {
    public:
      ScopedWeakValues(JsEngine* engine, const JsValueList& value);
      ~ScopedWeakValues();
      JsValueList Values() const;

    private:
      class RegisteredWeakValue
      {
      public:
        RegisteredWeakValue(JsEngine* engine, const JsValueList& val);
        ~RegisteredWeakValue();
        RegisteredWeakValue& operator=(const RegisteredWeakValue&) = delete;
        RegisteredWeakValue(const RegisteredWeakValue&) = delete;
        RegisteredWeakValue& operator=(RegisteredWeakValue&&) = delete;
        RegisteredWeakValue(RegisteredWeakValue&&) = delete;

        JsValueList Values() const;
        void Invalidate();

      private:
        JsEngine* engine;
        JsWeakValuesID weakId;
      };

      friend class JsEngine;
      std::shared_ptr<RegisteredWeakValue> state;
    };

    /**
     * Creates a new JavaScript engine instance.
     *
     * @param appInfo Information about the app.
     * @param interfaces contains implementation for the interfaces JsEngine uses.
     * @param isolate A provider of v8::Isolate, if the value is nullptr then
     *        a default implementation is used.
     * @return New `JsEngine` instance.
     */
    static std::unique_ptr<JsEngine> New(const AppInfo& appInfo,
                                         const Interfaces& interfaces,
                                         std::unique_ptr<IV8IsolateProvider> isolate = nullptr);

    /**
     * Registers the callback function for an event.
     * @param eventName Event name. Note that this can be any string - it's a
     *        general purpose event handling mechanism.
     * @param callback Event callback function.
     */
    void SetEventCallback(const std::string& eventName, const EventCallback& callback);

    /**
     * Removes the callback function for an event.
     * @param eventName Event name.
     */
    void RemoveEventCallback(const std::string& eventName);

    /**
     * Triggers an event.
     * @param eventName Event name.
     * @param params Event parameters.
     */
    void TriggerEvent(const std::string& eventName, JsValueList&& params);

    /**
     * Evaluates a JavaScript expression.
     * @param source JavaScript expression to evaluate.
     * @param filename Optional file name for the expression, used in error
     *        messages.
     * @return Result of the evaluated expression.
     */
    JsValue Evaluate(const std::string& source, const std::string& filename = "");

    /**
     * Initiates a garbage collection.
     */
    void Gc();

    //@{
    /**
     * Creates a new JavaScript value.
     * @param val Value to convert.
     * @return New `JsValue` instance.
     */
    JsValue NewValue(const std::string& val);
    JsValue NewValue(int64_t val);
    JsValue NewValue(bool val);
    JsValue NewValue(double val);
    inline JsValue NewValue(const char* val)
    {
      return NewValue(std::string(val));
    }
    inline JsValue NewValue(int val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#ifdef __APPLE__
    inline JsValue NewValue(long val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#endif
    //@}

    /**
     * Creates a new JavaScript object.
     * @return New `JsValue` instance.
     */
    JsValue NewObject();

    /**
     * Creates a new JavaScript array of strings.
     * @return New `JsValue` instance.
     */
    JsValue NewArray(const std::vector<std::string>& values);

    /**
     * Creates a JavaScript function that invokes a C++ callback.
     * @param callback C++ callback to invoke. The callback receives a
     *        `v8::FunctionCallbackInfo` object and can use `FromArguments()` to retrieve
     *        the current `JsEngine`.
     * @return New `JsValue` instance.
     */
    JsValue NewCallback(const v8::FunctionCallback& callback);

    /**
     * Returns a `JsEngine` instance contained in a `v8::FunctionCallbackInfo` object.
     * Use this in callbacks created via `NewCallback()` to retrieve the current
     * `JsEngine`.
     * @param arguments `v8::FunctionCallbackInfo` object containing the `JsEngine`
     *        instance.
     * @return `JsEngine` instance from `v8::FunctionCallbackInfo`.
     */
    static JsEngine* FromArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /*
     * Private functionality required to implement timers.
     * @param arguments `v8::FunctionCallbackInfo` is the arguments received in C++
     * callback associated for global setTimeout method.
     */
    static void ScheduleTimer(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * Private functionality required to implement web requests.
     * @param method `WebRequestMethod` is a request method.
     * @param arguments `v8::FunctionCallbackInfo` is the arguments received in C++
     * callback associated for global GET method.
     */
    static void ScheduleWebRequest(WebRequestMethod method, const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * Converts v8 arguments to `JsValue` objects.
     * @param arguments `v8::FunctionCallbackInfo` object containing the arguments to
     *        convert.
     * @return List of arguments converted to `const JsValue` objects.
     */
    JsValueList ConvertArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * Sets a global property that can be accessed by all the scripts.
     * @param name Name of the property to set.
     * @param value Value of the property to set.
     */
    void SetGlobalProperty(const std::string& name, const AdblockPlus::JsValue& value);

    v8::Isolate* GetIsolate() const
    {
      return isolate->Get();
    }

    v8::Global<v8::Context>* GetContext();

    /**
     * Notifies JS engine about critically low memory what should cause a
     * garbage collection.
     */
    void NotifyLowMemory();

    ITimer& GetTimer() const
    {
      return timer;
    }

    IFileSystem& GetFileSystem() const
    {
      return fileSystem;
    }

    IWebRequest& GetWebRequest() const
    {
      return webRequest;
    }

    LogSystem& GetLogSystem() const
    {
      return logSystem;
    }

    IResourceReader& GetResourceReader() const
    {
      return resourceReader;
    }

  private:
    void CallTimerTask(const JsWeakValuesID& timerParamsID);

    JsEngine(const Interfaces& interfaces, std::unique_ptr<IV8IsolateProvider> isolate);

    JsValue GetGlobalObject();
    friend class ScopedWeakValues::RegisteredWeakValue;
    JsWeakValuesID StoreJsValues(const JsValueList& values);
    JsValueList TakeJsValues(const JsWeakValuesID& id);
    JsValueList GetJsValues(const JsWeakValuesID& id);
    void RegisterScopedWeakValue(ScopedWeakValues::RegisteredWeakValue* value);
    void UnregisterScopedWeakValue(ScopedWeakValues::RegisteredWeakValue* value);

    ITimer& timer;
    IFileSystem& fileSystem;
    IWebRequest& webRequest;
    LogSystem& logSystem;
    IResourceReader& resourceReader;

#if !defined(MAKE_ISOLATE_IN_JS_VALUE_WEAK)
    /// Isolate must be disposed only after disposing of all objects which are
    /// using it.
    std::unique_ptr<IV8IsolateProvider> isolate;
#else
    // Due to lack of control on Java side this is needed to make JsValue dtor
    // not crashing.
    class IV8IsolateProviderWrapper : public IV8IsolateProvider
    {
    public:
      explicit IV8IsolateProviderWrapper(std::weak_ptr<IV8IsolateProvider> weakIsolate);
      v8::Isolate* Get() override;

    private:
      std::weak_ptr<IV8IsolateProvider> isolate;
    };

    // This is shared only to be able to create weak_ptrs of it.
    // It's not really shared i.e. passed as shared anywhere.
    std::shared_ptr<IV8IsolateProvider> isolate;
#endif

    IV8IsolateProviderPtr GetIsolateProviderPtr() const;

    v8::Global<v8::Context> context;
    EventMap eventCallbacks;
    std::mutex eventCallbacksMutex;
    JsWeakValuesLists jsWeakValuesLists;
    std::mutex jsWeakValuesListsMutex;
    std::vector<ScopedWeakValues::RegisteredWeakValue*> registeredWeakValues;
  };
}
