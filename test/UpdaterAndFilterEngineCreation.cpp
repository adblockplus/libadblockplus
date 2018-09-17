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

#include <string>
#include <memory>
#include <array>

#include <AdblockPlus/AsyncExecutor.h>

#include "BaseJsTest.h"

using namespace AdblockPlus;

namespace
{
  class UpdaterAndFilterEngineCreationTest : public BaseJsTest
  {
  protected:

    static const size_t COUNT = 100;
    const std::string PROP_NAME = "patternsbackupinterval";
    std::array<Updater*, COUNT> updaterAddrArray;
    std::array<FilterEngine*, COUNT> filterAddrArray;
    DelayedWebRequest::SharedTasks webRequestTasks;
    DelayedTimer::SharedTasks timerTasks;

    void SetUp()
    {
      LazyFileSystem* fileSystem;
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer = DelayedTimer::New(timerTasks);
      platformParams.fileSystem.reset(fileSystem = new LazyFileSystem());
      platformParams.webRequest = DelayedWebRequest::New(webRequestTasks);
      platform.reset(new Platform(std::move(platformParams)));
      std::uninitialized_fill(updaterAddrArray.begin(), updaterAddrArray.end(), nullptr);
      std::uninitialized_fill(filterAddrArray.begin(), filterAddrArray.end(), nullptr);
    }

    void CallGetUpdaterSimultaneously()
    {
      CallGetSimultaneously(true, false);
    }

    void CallGetFilterEngineSimultaneously()
    {
      CallGetSimultaneously(false, true);
    }

    void CallGetUpdaterAndGetFilterEngineSimultaneously()
    {
      CallGetSimultaneously(true, true);
    }

  private:

    void CallGetSimultaneously(bool isUpdater, bool isFilterEngine)
    {
      AsyncExecutor asyncExecutor;
      for (size_t idx = 0; idx < COUNT; ++idx)
        asyncExecutor.Dispatch(([this, idx, isUpdater, isFilterEngine]() -> void {
          if (isUpdater && isFilterEngine)
          {
            // some randomization in order of calling gets
            if (idx % 2)
            {
              updaterAddrArray[idx] = &(platform->GetUpdater());
              filterAddrArray[idx] = &(platform->GetFilterEngine());
            }
            else
            {
              filterAddrArray[idx] = &(platform->GetFilterEngine());
              updaterAddrArray[idx] = &(platform->GetUpdater());
            }
          }
          else if (isUpdater)
            updaterAddrArray[idx] = &(platform->GetUpdater());
          else if (isFilterEngine)
            filterAddrArray[idx] = &(platform->GetFilterEngine());
        }));
    }
  };
}

TEST_F(UpdaterAndFilterEngineCreationTest, TestFilterEngineSingleInstance)
{
    CallGetFilterEngineSimultaneously();
    FilterEngine* filterEngineAddr = filterAddrArray[0];
    EXPECT_NE(nullptr, filterEngineAddr);
    for (size_t i = 1; i < COUNT; ++i)
      ASSERT_EQ(filterEngineAddr, filterAddrArray[i]);
}

TEST_F(UpdaterAndFilterEngineCreationTest, TestUpdaterSingleInstance)
{
    CallGetUpdaterSimultaneously();
    Updater* updaterAddr = updaterAddrArray[0];
    EXPECT_NE(nullptr, updaterAddr);
    for (size_t i = 1; i < COUNT; ++i)
      ASSERT_EQ(updaterAddr, updaterAddrArray[i]);
}

TEST_F(UpdaterAndFilterEngineCreationTest, TestUpdaterAndFilterEngineCreationsDontCollide)
{
    CallGetUpdaterAndGetFilterEngineSimultaneously();
    EXPECT_NE(nullptr, filterAddrArray[0]);
    EXPECT_NE(nullptr, updaterAddrArray[0]);
    EXPECT_EQ(filterAddrArray[0], filterAddrArray[COUNT - 1]);
    EXPECT_EQ(updaterAddrArray[0], updaterAddrArray[COUNT - 1]);
}

TEST_F(UpdaterAndFilterEngineCreationTest, TestUpdaterAndFilterEngineCreationOrder1)
{
    Updater& updater = platform->GetUpdater();
    FilterEngine& filterEngine = platform->GetFilterEngine();

    int propFromUpdater = updater.GetPref(PROP_NAME).AsInt();
    int propFromFilterEngine = filterEngine.GetPref(PROP_NAME).AsInt();
    EXPECT_EQ(propFromUpdater, propFromFilterEngine);

    int newPropValue = 8;
    updater.SetPref(PROP_NAME, GetJsEngine().NewValue(newPropValue));
    propFromUpdater = updater.GetPref(PROP_NAME).AsInt();
    propFromFilterEngine = filterEngine.GetPref(PROP_NAME).AsInt();
    EXPECT_EQ(newPropValue, propFromUpdater);
    EXPECT_EQ(newPropValue, propFromFilterEngine);
}

TEST_F(UpdaterAndFilterEngineCreationTest, TestUpdaterAndFilterEngineCreationOrder2)
{
    FilterEngine& filterEngine = platform->GetFilterEngine();
    Updater& updater = platform->GetUpdater();

    int propFromFilterEngine = filterEngine.GetPref(PROP_NAME).AsInt();
    int propFromUpdater = updater.GetPref(PROP_NAME).AsInt();
    EXPECT_EQ(propFromUpdater, propFromFilterEngine);

    int newPropValue = 18;
    filterEngine.SetPref(PROP_NAME, GetJsEngine().NewValue(newPropValue));
    propFromFilterEngine = filterEngine.GetPref(PROP_NAME).AsInt();
    propFromUpdater = updater.GetPref(PROP_NAME).AsInt();
    EXPECT_EQ(newPropValue, propFromUpdater);
    EXPECT_EQ(newPropValue, propFromFilterEngine);
}
