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

#ifndef ADBLOCK_PLUS_DEFAULT_PLATFORM_H
#define ADBLOCK_PLUS_DEFAULT_PLATFORM_H

#include <AdblockPlus/IExecutor.h>
#include <AdblockPlus/PlatformFactory.h>

namespace AdblockPlus
{
  class DefaultPlatform : public Platform
  {
  public:
    DefaultPlatform(PlatformFactory::CreationParameters&& creationParameters);
    ~DefaultPlatform() override;

    JsEngine& GetJsEngine();

    void SetUp(const AppInfo& appInfo = AppInfo(),
               std::unique_ptr<IV8IsolateProvider> isolate = nullptr) override;

    void CreateFilterEngineAsync(
        const FilterEngineFactory::CreationParameters& parameters =
            FilterEngineFactory::CreationParameters(),
        const OnFilterEngineCreatedCallback& onCreated = OnFilterEngineCreatedCallback()) override;

    IFilterEngine& GetFilterEngine() override;
    void WithTimer(const WithTimerCallback&) override;
    void WithFileSystem(const WithFileSystemCallback&) override;
    void WithWebRequest(const WithWebRequestCallback&) override;
    void WithLogSystem(const WithLogSystemCallback&) override;
    void WithResourceReader(const WithResourceReaderCallback&) override;

  protected:
    LogSystemPtr logSystem;
    TimerPtr timer;
    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    std::unique_ptr<IResourceReader> resourceReader;

  private:
    std::unique_ptr<JsEngine> jsEngine;
    std::unique_ptr<IExecutor> executor;
    std::recursive_mutex interfacesMutex;
    // used for creation and deletion of modules.
    std::mutex modulesMutex;
    std::shared_future<std::unique_ptr<IFilterEngine>> filterEngine;
    std::set<std::string> evaluatedJsSources;
    std::mutex evaluatedJsSourcesMutex;

    std::function<void(const std::string&)> GetEvaluateCallback();
  };
}

#endif // ADBLOCK_PLUS_DEFAULT_PLATFORM_H
