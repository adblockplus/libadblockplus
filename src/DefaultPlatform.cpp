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

#include "DefaultPlatform.h"

#include <cassert>

#include "JsEngine.h"

using namespace AdblockPlus;

extern std::string jsSources[];

namespace
{
  template<typename T>
  void ValidatePlatformCreationParameter(const std::unique_ptr<T>& param, const char* paramName)
  {
    if (!param)
      throw std::logic_error(paramName + std::string(" must not be nullptr"));
  }
}

DefaultPlatform::DefaultPlatform(PlatformFactory::CreationParameters&& creationParameters)
{
#define ASSIGN_PLATFORM_PARAM(param)                                                               \
  ValidatePlatformCreationParameter(param = std::move(creationParameters.param), #param)

  ASSIGN_PLATFORM_PARAM(logSystem);
  ASSIGN_PLATFORM_PARAM(timer);
  ASSIGN_PLATFORM_PARAM(fileSystem);
  ASSIGN_PLATFORM_PARAM(webRequest);
  ASSIGN_PLATFORM_PARAM(resourceReader);
  ASSIGN_PLATFORM_PARAM(executor);

#undef ASSIGN_PLATFORM_PARAM
}

DefaultPlatform::~DefaultPlatform()
{
  executor->Stop();
  LogSystemPtr tmpLogSystem;
  TimerPtr tmpTimer;
  FileSystemPtr tmpFileSystem;
  WebRequestPtr tmpWebRequest;
  std::unique_ptr<IResourceReader> tmpResourceReader;
  {
    std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
    tmpLogSystem = std::move(logSystem);
    tmpTimer = std::move(timer);
    tmpFileSystem = std::move(fileSystem);
    tmpWebRequest = std::move(webRequest);
    tmpResourceReader = std::move(resourceReader);
  }
}

JsEngine& DefaultPlatform::GetJsEngine()
{
  SetUp();
  return *jsEngine;
}

void DefaultPlatform::SetUp(const AppInfo& appInfo, std::unique_ptr<IV8IsolateProvider> isolate)
{
  std::lock_guard<std::mutex> lock(modulesMutex);
  if (jsEngine)
    return;
  jsEngine = JsEngine::New(appInfo, *this, std::move(isolate));
}

void DefaultPlatform::CreateFilterEngineAsync(
    const FilterEngineFactory::CreationParameters& parameters,
    const Platform::OnFilterEngineCreatedCallback& onCreated)
{
  std::shared_ptr<std::promise<std::unique_ptr<IFilterEngine>>> filterEnginePromise;
  {
    std::lock_guard<std::mutex> lock(modulesMutex);
    if (filterEngine.valid())
      return;
    filterEnginePromise = std::make_shared<std::promise<std::unique_ptr<IFilterEngine>>>();
    filterEngine = filterEnginePromise->get_future();
  }

  GetJsEngine(); // ensures that JsEngine is instantiated
  FilterEngineFactory::CreateAsync(
      *jsEngine,
      GetEvaluateCallback(),
      [onCreated, filterEnginePromise](std::unique_ptr<IFilterEngine> filterEngine) {
        const auto& filterEngineRef = *filterEngine;
        filterEnginePromise->set_value(std::move(filterEngine));
        if (onCreated)
          onCreated(filterEngineRef);
      },
      parameters);
}

IFilterEngine& DefaultPlatform::GetFilterEngine()
{
  CreateFilterEngineAsync();
  return *filterEngine.get().get();
}

void DefaultPlatform::WithTimer(const WithTimerCallback& callback)
{
  std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
  if (timer && callback)
    callback(*timer);
}

void DefaultPlatform::WithFileSystem(const WithFileSystemCallback& callback)
{
  std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
  if (fileSystem && callback)
    callback(*fileSystem);
}

void DefaultPlatform::WithWebRequest(const WithWebRequestCallback& callback)
{
  std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
  if (webRequest && callback)
    callback(*webRequest);
}

void DefaultPlatform::WithLogSystem(const WithLogSystemCallback& callback)
{
  std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
  if (logSystem && callback)
    callback(*logSystem);
}

void DefaultPlatform::WithResourceReader(const WithResourceReaderCallback& callback)
{
  std::lock_guard<std::recursive_mutex> lock(interfacesMutex);
  if (resourceReader && callback)
    callback(*resourceReader);
}

std::function<void(const std::string&)> DefaultPlatform::GetEvaluateCallback()
{
  // GetEvaluateCallback() method assumes that jsEngine is already created
  return [this](const std::string& filename) {
    std::lock_guard<std::mutex> lock(evaluatedJsSourcesMutex);
    if (evaluatedJsSources.find(filename) != evaluatedJsSources.end())
      return; // NO-OP, file was already evaluated

    for (int i = 0; !jsSources[i].empty(); i += 2)
      if (jsSources[i] == filename)
      {
        jsEngine->Evaluate(jsSources[i + 1], jsSources[i]);
        evaluatedJsSources.insert(filename);
        break;
      }
  };
}
