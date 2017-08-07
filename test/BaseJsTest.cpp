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

#include "BaseJsTest.h"
#include <AdblockPlus/FilterEngine.h>

using namespace AdblockPlus;

void DelayedTimer::ProcessImmediateTimers(DelayedTimer::SharedTasks& timerTasks)
{
  auto ii = timerTasks->begin();
  while (ii != timerTasks->end())
  {
    if (ii->timeout.count() == 0)
    {
      ii->callback();
      ii = timerTasks->erase(ii);
    }
    else
      ++ii;
  }
}

FilterEnginePtr CreateFilterEngine(LazyFileSystem& fileSystem,
  Platform& platform,
  const FilterEngine::CreationParameters& creationParams)
{
  std::list<LazyFileSystem::Task> fileSystemTasks;
  fileSystem.scheduler = [&fileSystemTasks](const LazyFileSystem::Task& task)
  {
    fileSystemTasks.emplace_back(task);
  };
  FilterEnginePtr retValue;
  platform.CreateFilterEngineAsync(creationParams, [&retValue, &fileSystem](const FilterEnginePtr& filterEngine)
  {
    retValue = filterEngine;
    fileSystem.scheduler = LazyFileSystem::ExecuteImmediately;
  });
  while (!retValue && !fileSystemTasks.empty())
  {
    (*fileSystemTasks.begin())();
    fileSystemTasks.pop_front();
  }
  return retValue;
}

ThrowingPlatformCreationParameters::ThrowingPlatformCreationParameters()
{
  logSystem.reset(new ThrowingLogSystem());
  timer.reset(new ThrowingTimer());
  fileSystem = std::make_shared<ThrowingFileSystem>();
  webRequest.reset(new ThrowingWebRequest());
}