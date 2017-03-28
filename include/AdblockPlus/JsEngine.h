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
#include <AdblockPlus/FileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>
#include <AdblockPlus/ITimer.h>

namespace v8
{
  class Arguments;
  class Isolate;
  class Value;
  class Context;
  template<class T> class Handle;
  typedef Handle<Value>(*InvocationCallback)(const Arguments &args);
}

namespace AdblockPlus
{
  class JsEngine;

  /**
   * Shared smart pointer to a `JsEngine` instance.
   */
  typedef std::shared_ptr<JsEngine> JsEnginePtr;

  /**
   * A factory to construct DefaultTimer.
   */
  TimerPtr CreateDefaultTimer();

  /**
   * Scope based isolate manager. Creates a new isolate instance on
   * constructing and disposes it on destructing.
   */
  class ScopedV8Isolate
  {
  public:
    ScopedV8Isolate();
    ~ScopedV8Isolate();
    v8::Isolate* Get()
    {
      return isolate;
    }
  private:
    ScopedV8Isolate(const ScopedV8Isolate&);
    ScopedV8Isolate& operator=(const ScopedV8Isolate&);

    v8::Isolate* isolate;
  };

  /**
   * Shared smart pointer to ScopedV8Isolate instance;
   */
  typedef std::shared_ptr<ScopedV8Isolate> ScopedV8IsolatePtr;

  /**
   * JavaScript engine used by `FilterEngine`, wraps v8.
   */
  class JsEngine : public std::enable_shared_from_this<JsEngine>
  {
    friend class JsValue;
    friend class JsContext;

  public:
    /**
     * Event callback function.
     */
    typedef std::function<void(JsValueList& params)> EventCallback;

    /**
    * Callback function returning false when current connection is not allowed
    * e.g. because it is a metered connection.
    */
    typedef std::function<bool()> IsConnectionAllowedCallback;

    /**
     * Maps events to callback functions.
     */
    typedef std::map<std::string, EventCallback> EventMap;

    /**
     * Creates a new JavaScript engine instance.
     * @param appInfo Information about the app.
     * @param timer Implementation of timer.
     * @param isolate v8::Isolate wrapper. This parameter should be considered
     *        as a temporary hack for tests, it will go away. Issue #3593.
     * @return New `JsEngine` instance.
     */
    static JsEnginePtr New(const AppInfo& appInfo = AppInfo(),
      TimerPtr timer = CreateDefaultTimer(),
      const ScopedV8IsolatePtr& isolate = ScopedV8IsolatePtr(new ScopedV8Isolate()));

    /**
     * Registers the callback function for an event.
     * @param eventName Event name. Note that this can be any string - it's a
     *        general purpose event handling mechanism.
     * @param callback Event callback function.
     */
    void SetEventCallback(const std::string& eventName, EventCallback callback);

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
    void TriggerEvent(const std::string& eventName, JsValueList& params);

    /**
     * Evaluates a JavaScript expression.
     * @param source JavaScript expression to evaluate.
     * @param filename Optional file name for the expression, used in error
     *        messages.
     * @return Result of the evaluated expression.
     */
    JsValuePtr Evaluate(const std::string& source,
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
    //@}

    /**
     * Creates a new JavaScript object.
     * @return New `JsValue` instance.
     */
    JsValuePtr NewObject();

    /**
     * Creates a JavaScript function that invokes a C++ callback.
     * @param callback C++ callback to invoke. The callback receives a
     *        `v8::Arguments` object and can use `FromArguments()` to retrieve
     *        the current `JsEngine`.
     * @return New `JsValue` instance.
     */
    JsValuePtr NewCallback(v8::InvocationCallback callback);

    /**
     * Returns a `JsEngine` instance contained in a `v8::Arguments` object.
     * Use this in callbacks created via `NewCallback()` to retrieve the current
     * `JsEngine`.
     * @param arguments `v8::Arguments` object containing the `JsEngine`
     *        instance.
     * @return `JsEngine` instance from `v8::Arguments`.
     */
    static JsEnginePtr FromArguments(const v8::Arguments& arguments);

    /*
     * Private functionality required to implement timers.
     * @param arguments `v8::Arguments` is the arguments received in C++
     * callback associated for global setTimeout method.
     */
    static void ScheduleTimer(const v8::Arguments& arguments);

    /**
     * Converts v8 arguments to `JsValue` objects.
     * @param arguments `v8::Arguments` object containing the arguments to
     *        convert.
     * @return List of arguments converted to `JsValue` objects.
     */
    JsValueList ConvertArguments(const v8::Arguments& arguments);

    /**
     * @see `SetFileSystem()`.
     */
    FileSystemPtr GetFileSystem();

    /**
     * Sets the `FileSystem` implementation used for all file I/O.
     * Setting this is optional, the engine will use a `DefaultFileSystem`
     * instance by default, which might be sufficient.
     * @param The `FileSystem` instance to use.
     */
    void SetFileSystem(FileSystemPtr val);

    /**
     * @see `SetWebRequest()`.
     */
    WebRequestPtr GetWebRequest();

    /**
     * Sets the `WebRequest` implementation used for XMLHttpRequests.
     * Setting this is optional, the engine will use a `DefaultWebRequest`
     * instance by default, which might be sufficient.
     * @param The `WebRequest` instance to use.
     */
    void SetWebRequest(WebRequestPtr val);

    /**
    * Registers the callback function to check whether current connection is
    * allowed for network requests.
    * @param callback callback function.
    */
    void SetIsConnectionAllowedCallback(const IsConnectionAllowedCallback& callback);

    /**
     * Checks whether current connection is allowed. If
     * IsConnectionAllowedCallback is not set then then it returns true.
     */
    bool IsConnectionAllowed();

    /**
     * @see `SetLogSystem()`.
     */
    LogSystemPtr GetLogSystem();

    /**
     * Sets the `LogSystem` implementation used for logging (e.g. to handle
     * `console.log()` calls from JavaScript).
     * Setting this is optional, the engine will use a `DefaultLogSystem`
     * instance by default, which might be sufficient.
     * @param The `LogSystem` instance to use.
     */
    void SetLogSystem(LogSystemPtr val);

    /**
     * Sets a global property that can be accessed by all the scripts.
     * @param name Name of the property to set.
     * @param value Value of the property to set.
     */
    void SetGlobalProperty(const std::string& name, AdblockPlus::JsValuePtr value);

    /**
     * Returns a pointer to associated v8::Isolate.
     */
    v8::Isolate* GetIsolate()
    {
      return isolate->Get();
    }

  private:
    struct TimerTask
    {
      ~TimerTask();
      std::vector<std::unique_ptr<v8::Persistent<v8::Value>>> arguments;
    };
    typedef std::list<TimerTask> TimerTasks;
    void CallTimerTask(TimerTasks::const_iterator timerTaskIterator);

    explicit JsEngine(const ScopedV8IsolatePtr& isolate, TimerPtr timer);

    JsValuePtr GetGlobalObject();

    /// Isolate must be disposed only after disposing of all objects which are
    /// using it.
    ScopedV8IsolatePtr isolate;

    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    LogSystemPtr logSystem;
    std::unique_ptr<v8::Persistent<v8::Context>> context;
    EventMap eventCallbacks;
    std::mutex eventCallbacksMutex;
    std::mutex isConnectionAllowedMutex;
    IsConnectionAllowedCallback isConnectionAllowed;
    TimerTasks timerTasks;
    TimerPtr timer;
  };
}

#endif
