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

#ifndef ADBLOCK_PLUS_PLATFORM_H
#define ADBLOCK_PLUS_PLATFORM_H

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/FilterEngineFactory.h>
#include <AdblockPlus/IFileSystem.h>
#include <AdblockPlus/IResourceReader.h>
#include <AdblockPlus/ITimer.h>
#include <AdblockPlus/IWebRequest.h>
#include <AdblockPlus/LogSystem.h>

namespace AdblockPlus
{
  struct IV8IsolateProvider;

  /**
   * AdblockPlus platform is the main component providing access to other
   * modules.
   *
   * It manages the functionality modules, e.g. JsEngine and IFilterEngine as
   * well as allows to correctly work with asynchronous functionality.
   * It is expected that other objects returned by the API, such as Filter, Subscription
   * and JsValue, will be released before the Platform.
   */
  class Platform
  {
  public:
    /**
     * Callback type invoked when IFilterEngine is created.
     */
    typedef std::function<void(const IFilterEngine&)> OnFilterEngineCreatedCallback;

    virtual ~Platform() = default;

    /**
     * Ensures that Platform is ready. If method was already called
     * then the parameters are ignored.
     *
     * @param appInfo Information about the app,
     * @param isolate A provider of v8::Isolate, if the value is nullptr then
     *        a default implementation is used.
     */
    virtual void SetUp(const AppInfo& appInfo = AppInfo(),
                       std::unique_ptr<IV8IsolateProvider> isolate = nullptr) = 0;

    /**
     * Ensures that IFilterEngine is constructed. Only the first call is effective.
     *
     * @param parameters optional creation parameters.
     * @param onCreated A callback which is called when IFilterEngine is ready
     *        for use.
     */
    virtual void CreateFilterEngineAsync(
        const FilterEngineFactory::CreationParameters& parameters =
            FilterEngineFactory::CreationParameters(),
        const OnFilterEngineCreatedCallback& onCreated = OnFilterEngineCreatedCallback()) = 0;

    /**
     * Synchronous equivalent of `CreateFilterEngineAsync`.
     * Internally it blocks and waits for finishing of certain asynchronous
     * operations, please ensure that provided implementation does not lead to
     * a dead lock.
     */
    virtual IFilterEngine& GetFilterEngine() = 0;

    typedef std::function<void(ITimer&)> WithTimerCallback;
    virtual void WithTimer(const WithTimerCallback&) = 0;

    typedef std::function<void(IFileSystem&)> WithFileSystemCallback;
    virtual void WithFileSystem(const WithFileSystemCallback&) = 0;

    typedef std::function<void(IWebRequest&)> WithWebRequestCallback;
    virtual void WithWebRequest(const WithWebRequestCallback&) = 0;

    typedef std::function<void(LogSystem&)> WithLogSystemCallback;
    virtual void WithLogSystem(const WithLogSystemCallback&) = 0;

    typedef std::function<void(IResourceReader&)> WithResourceReaderCallback;
    virtual void WithResourceReader(const WithResourceReaderCallback&) = 0;
  };
}

#endif // ADBLOCK_PLUS_PLATFORM_H
