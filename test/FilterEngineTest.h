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

#include "../src/DefaultLogSystem.h"
#include "../src/DefaultResourceReader.h"
#include "BaseJsTest.h"

namespace AdblockPlus
{
  namespace Utils
  {
    inline bool BeginsWith(const std::string& str, const std::string& beginning)
    {
      return 0 == str.compare(0, beginning.size(), beginning);
    }
  }
}

class NoFilesFileSystem : public LazyFileSystem
{
public:
  void Stat(const std::string& fileName, const StatCallback& callback) const override
  {
    scheduler([callback] {
      callback(StatResult(), "");
    });
  }
};

template<class LazyFileSystemT, class LogSystem> class FilterEngineTestGeneric : public BaseJsTest
{
public:
protected:
  void SetUp() override
  {
    LazyFileSystemT* fileSystem;
    ThrowingPlatformCreationParameters platformParams;
    platformParams.logSystem.reset(new LogSystem());
    platformParams.timer.reset(new NoopTimer());
    platformParams.fileSystem.reset(fileSystem = new LazyFileSystemT());
    platformParams.webRequest.reset(new NoopWebRequest());
    platformParams.resourceReader.reset(new AdblockPlus::DefaultResourceReader());
    platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(platformParams));
    ::CreateFilterEngine(*platform);
  }

  AdblockPlus::IFilterEngine& GetFilterEngine()
  {
    return platform->GetFilterEngine();
  }
};

typedef FilterEngineTestGeneric<LazyFileSystem, AdblockPlus::DefaultLogSystem> FilterEngineTest;
typedef FilterEngineTestGeneric<NoFilesFileSystem, LazyLogSystem> FilterEngineTestNoData;

class FilterEngineWithInMemoryFS : public BaseJsTest
{
protected:
  void InitPlatformAndAppInfo(AdblockPlus::PlatformFactory::CreationParameters&& params =
                                  AdblockPlus::PlatformFactory::CreationParameters(),
                              const AdblockPlus::AppInfo& appInfo = AdblockPlus::AppInfo())
  {
    if (!params.logSystem)
      params.logSystem.reset(new AdblockPlus::DefaultLogSystem());
    if (!params.timer)
      params.timer.reset(new NoopTimer());
    if (!params.fileSystem)
      params.fileSystem.reset(new InMemoryFileSystem());
    if (!params.webRequest)
      params.webRequest.reset(new NoopWebRequest());
    if (!params.resourceReader)
      params.resourceReader.reset(new AdblockPlus::DefaultResourceReader());
    platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(params));
    platform->SetUp(appInfo);
  }

  AdblockPlus::IFilterEngine&
  CreateFilterEngine(const AdblockPlus::FilterEngineFactory::CreationParameters& creationParams =
                         AdblockPlus::FilterEngineFactory::CreationParameters())
  {
    ::CreateFilterEngine(*platform, creationParams);
    return platform->GetFilterEngine();
  }
};

class FilterEngineInitallyDisabledTest : public FilterEngineWithInMemoryFS
{
public:
  FilterEngineInitallyDisabledTest()
  {
  }

protected:
  int webRequestCounter;
  std::string filterList;

  enum class AutoselectState
  {
    Enabled,
    Disabled
  };

  enum class EngineState
  {
    Enabled,
    Disabled
  };

  void SetUp() override
  {
    filterList = "[Adblock Plus 2.0]\n||example.com";
    webRequestCounter = 0;
  }

  AdblockPlus::IFilterEngine&
  ConfigureEngine(AutoselectState autoselectState,
                  EngineState engineState,
                  AdblockPlus::PlatformFactory::CreationParameters&& params =
                      AdblockPlus::PlatformFactory::CreationParameters())
  {
    auto impl = [this](const std::string&,
                       const AdblockPlus::HeaderList&,
                       const AdblockPlus::IWebRequest::RequestCallback& callback) {
      ++webRequestCounter;
      AdblockPlus::ServerResponse response;
      response.responseStatus = 200;
      response.status = AdblockPlus::IWebRequest::NS_OK;
      response.responseText = filterList;
      callback(response);
    };

    {
      AdblockPlus::AppInfo info;
      info.locale = "en";
      params.webRequest.reset(new WrappingWebRequest(impl, impl));
      params.logSystem.reset(new AdblockPlus::DefaultLogSystem());
      InitPlatformAndAppInfo(std::move(params), info);
    }

    AdblockPlus::FilterEngineFactory::CreationParameters createParams;
    createParams.preconfiguredPrefs.booleanPrefs.emplace(
        AdblockPlus::FilterEngineFactory::BooleanPrefName::FilterEngineEnabled,
        engineState == EngineState::Enabled);
    createParams.preconfiguredPrefs.booleanPrefs.emplace(
        AdblockPlus::FilterEngineFactory::BooleanPrefName::FirstRunSubscriptionAutoselect,
        autoselectState == AutoselectState::Enabled);
    return CreateFilterEngine(createParams);
  }
};
