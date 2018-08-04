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

#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <functional>
#include <map>
#include <list>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <mutex>
#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/LogSystem.h>
#include <AdblockPlus/IFileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/IWebRequest.h>
#include <AdblockPlus/ITimer.h>
#include <AdblockPlus/Scheduler.h>

namespace v8
{
  class Isolate;
  class Value;
  class Context;
  template<typename T> class FunctionCallbackInfo;
  typedef void(*FunctionCallback)(const FunctionCallbackInfo<Value>& info);
}

namespace AdblockPlus
{
  class JsEngine;
  class Platform;

  /**
   * Shared smart pointer to a `JsEngine` instance.
   */
  typedef std::shared_ptr<JsEngine> JsEnginePtr;

  /**
   * Provides with isolate. The main aim of this iterface is to delegate a
   * proper initialization and deinitialization of v8::Isolate to an embedder.
   */
  struct IV8IsolateProvider
  {
    virtual ~IV8IsolateProvider() {}

    /**
     * Returns v8::Isolate. All subsequent calls of this method should return
     * the same pointer to v8::Isolate as the first call.
     */
    virtual v8::Isolate* Get() = 0;
  };

  /**
   * JavaScript engine used by `FilterEngine`, wraps v8.
   */
  class JsEngine : public std::enable_shared_from_this<JsEngine>
  {
    friend class JsValue;
    friend class JsContext;

    struct JsWeakValuesList
    {
      ~JsWeakValuesList();
      std::vector<v8::Global<v8::Value>> values;
    };
    typedef std::list<JsWeakValuesList> JsWeakValuesLists;
  public:
    /**
     * Event callback function.
     */
    typedef std::function<void(JsValueList&& params)> EventCallback;

    /**
     * Maps events to callback functions.
     */
    typedef std::map<std::string, EventCallback> EventMap;

    /**
     * An opaque structure representing ID of stored JsValueList.
     */
    class JsWeakValuesID
    {
      friend class JsEngine;
      JsWeakValuesLists::const_iterator iterator;
    };

    /**
     * Creates a new JavaScript engine instance.
     *
     * @param appInfo Information about the app.
     * @param platform AdblockPlus platform providing with necessary
     *        dependencies.
     * @param isolate A provider of v8::Isolate, if the value is nullptr then
     *        a default implementation is used.
     * @return New `JsEngine` instance.
     */
    static JsEnginePtr New(const AppInfo& appInfo, Platform& platform, std::unique_ptr<IV8IsolateProvider> isolate = nullptr);
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
    JsValue Evaluate(const std::string& source,
        const std::string& filename = "");

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
    static JsEnginePtr FromArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * Stores `JsValue`s in a way they don't keep a strong reference to
     * `JsEngine` and which are destroyed when `JsEngine` is destroyed. These
     * methods should be used when one needs to carry a JsValue in a callback
     * directly or indirectly passed to `JsEngine`.
     * The method is thread-safe.
     * @param `JsValueList` to store.
     * @return `JsWeakValuesID` of stored values which allows to restore them
     * later.
     */
    JsWeakValuesID StoreJsValues(const JsValueList& values);

    /**
     * Extracts and removes from `JsEngine` earlier stored `JsValue`s.
     * The method is thread-safe.
     * @param id `JsWeakValuesID` of values.
     * @return `JsValueList` of stored values.
     */
    JsValueList TakeJsValues(const JsWeakValuesID& id);

    /**
     * Extracts earlier stored `JsValue`s from `JsEgnine` but does not remove
     * them, one still must call `TakeJsValues`.
     * @param id `JsWeakValuesID` of values.
     * @return `JsValueList` of stored values.
     */
    JsValueList GetJsValues(const JsWeakValuesID& id);

    /*
     * Private functionality required to implement timers.
     * @param arguments `v8::FunctionCallbackInfo` is the arguments received in C++
     * callback associated for global setTimeout method.
     */
    static void ScheduleTimer(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /*
     * Private functionality required to implement web requests.
     * @param arguments `v8::FunctionCallbackInfo` is the arguments received in C++
     * callback associated for global GET method.
     */
    static void ScheduleWebRequest(const v8::FunctionCallbackInfo<v8::Value>& arguments);

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

    /**
     * Returns a pointer to associated v8::Isolate.
     */
    v8::Isolate* GetIsolate()
    {
      return isolate->Get();
    }

    /**
     * Notifies JS engine about critically low memory what should cause a
     * garbage collection.
     */
    void NotifyLowMemory();

    /**
     * Private functionality.
     */
    Platform& GetPlatform()
    {
      return platform;
    }
  private:
    void CallTimerTask(const JsWeakValuesID& timerParamsID);

    explicit JsEngine(Platform& platform, std::unique_ptr<IV8IsolateProvider> isolate);

    JsValue GetGlobalObject();

    Platform& platform;
    /// Isolate must be disposed only after disposing of all objects which are
    /// using it.
    std::unique_ptr<IV8IsolateProvider> isolate;

    std::unique_ptr<v8::Global<v8::Context>> context;
    EventMap eventCallbacks;
    std::mutex eventCallbacksMutex;
    JsWeakValuesLists jsWeakValuesLists;
    std::mutex jsWeakValuesListsMutex;
  };
}

#endif
