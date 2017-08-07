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

#ifndef ADBLOCK_PLUS_PLATFORM_H
#define ADBLOCK_PLUS_PLATFORM_H

#include "LogSystem.h"
#include "ITimer.h"
#include "IFileSystem.h"
#include "IWebRequest.h"
#include "AppInfo.h"
#include "Scheduler.h"
#include "FilterEngine.h"
#include <mutex>
#include <future>

namespace AdblockPlus
{
  /**
   * A factory to construct DefaultTimer.
   */
  TimerPtr CreateDefaultTimer();

  /**
   * A factory to construct DefaultFileSystem.
   */
  FileSystemPtr CreateDefaultFileSystem(const Scheduler& scheduler);

  /**
   * A factory to construct DefaultWebRequest.
   */
  WebRequestPtr CreateDefaultWebRequest(const Scheduler& scheduler);

  /**
   * A factory to construct LogSystem.
   */
  LogSystemPtr CreateDefaultLogSystem();

  class JsEngine;

  /**
   * AdblockPlus platform is the main component providing access to other
   * modules.
   *
   * It manages the functionality modules, e.g. JsEngine and FilterEngine as
   * well as allows to correctly work with asynchronous functionality.
   */
  class Platform
  {
  public:
    /**
     * Platform creation parameters.
     *
     * @param logSystem Implementation of log system.
     * @param timer Implementation of timer.
     * @param webRequest Implementation of web request.
     * @param fileSystem Implementation of filesystem.
     */
    struct CreationParameters
    {
      LogSystemPtr logSystem;
      TimerPtr timer;
      WebRequestPtr webRequest;
      FileSystemPtr fileSystem;
    };

    /**
     * Callback type invoked when FilterEngine is created.
     */
    typedef std::function<void(const FilterEnginePtr&)> OnFilterEngineCreatedCallback;

    /**
     * Platform constructor.
     *
     * When a parameter value is nullptr the corresponding default
     * implementation is chosen.
     */
    explicit Platform(CreationParameters&& creationParameters = CreationParameters());
    ~Platform();

    /**
     * Ensures that JsEngine is constructed.
     *
     * @param appInfo Information about the app, if jsEngine is already present
     *        then the value is ignored.
     */
    void SetUpJsEngine(const AppInfo& appInfo = AppInfo());

    /**
     * Retrieves the `JsEngine` instance. It calls SetUpJsEngine if JsEngine is
     * not initialized yet.
     */
    std::shared_ptr<JsEngine> GetJsEngine();

    /**
     * Ensures that FilterEngine is constructed. Only the first call is effective.
     *
     * @param parameters optional creation parameters.
     * @param onCreated A callback which is called when FilterEngine is ready
     *        for use.
     */
    void CreateFilterEngineAsync(const FilterEngine::CreationParameters& parameters = FilterEngine::CreationParameters(),
      const OnFilterEngineCreatedCallback& onCreated = OnFilterEngineCreatedCallback());

    /**
     * Synchronous equivalent of `CreateFilterEngineAsync`.
     * Internally it blocks and waits for finishing of certain asynchronous
     * operations, please ensure that provided implementation does not lead to
     * a dead lock.
     */
    FilterEnginePtr GetFilterEngine();

    /**
    * @return The asynchronous ITimer implementation.
     */
    ITimer& GetTimer();

    /**
     * @return The asynchronous IFileSystem implementation.
     */
    IFileSystem& GetFileSystem();

    /**
    * @return The asynchronous IWebRequest implementation.
    */
    IWebRequest& GetWebRequest();

    /**
     * @see `SetLogSystem()`.
     */
    LogSystem& GetLogSystem();

  private:
    LogSystemPtr logSystem;
    TimerPtr timer;
    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    // used for creation and deletion of modules.
    std::mutex modulesMutex;
    std::shared_ptr<JsEngine> jsEngine;
    std::shared_future<FilterEnginePtr> filterEngine;
  };
}

#endif // ADBLOCK_PLUS_PLATFORM_H
