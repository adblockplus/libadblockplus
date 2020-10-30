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

#include "BaseJsTest.h"

#include <AdblockPlus/IFilterEngine.h>

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

IFilterEngine& CreateFilterEngine(Platform& platform,
                                  const FilterEngineFactory::CreationParameters& creationParams)
{
  std::promise<void> promise;
  std::future<void> future = promise.get_future();

  platform.CreateFilterEngineAsync(creationParams,
                                   [&promise](const IFilterEngine& /*filterEngine*/) {
                                     promise.set_value();
                                   });

  future.wait();
  return platform.GetFilterEngine();
}

ThrowingPlatformCreationParameters::ThrowingPlatformCreationParameters()
{
  logSystem.reset(new ThrowingLogSystem());
  timer.reset(new ThrowingTimer());
  fileSystem.reset(new ThrowingFileSystem());
  webRequest.reset(new ThrowingWebRequest());
}