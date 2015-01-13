/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

#include <gtest/gtest.h>
#include <queue>
#include <vector>

#include "../src/Thread.h"

namespace
{
  class Mock : public AdblockPlus::Thread
  {
  public:
    int timesCalled;
    AdblockPlus::Mutex mutex;

    Mock() : timesCalled(0)
    {
    }

    void Run()
    {
      AdblockPlus::Sleep(5);
      AdblockPlus::Lock lock(mutex);
      timesCalled++;
    }
  };

  class LockingMock : public AdblockPlus::Thread
  {
  public:
    bool working;

    LockingMock(std::vector<std::string>& log, AdblockPlus::Mutex& logMutex)
      : log(log), logMutex(logMutex)
    {
    }

    void Run()
    {
      AdblockPlus::Lock lock(logMutex);
      log.push_back("started");
      AdblockPlus::Sleep(5);
      log.push_back("ended");
    }

  private:
    const std::string name;
    std::vector<std::string>& log;
    AdblockPlus::Mutex& logMutex;
  };
}

TEST(ThreadTest, Run)
{
  Mock mock;
  ASSERT_EQ(0, mock.timesCalled);
  mock.Start();
  mock.mutex.Lock();
  ASSERT_EQ(0, mock.timesCalled);
  mock.mutex.Unlock();
  mock.Join();
  ASSERT_EQ(1, mock.timesCalled);
}

TEST(ThreadTest, Mutex)
{
  std::vector<std::string> log;
  AdblockPlus::Mutex logMutex;
  LockingMock mock1(log, logMutex);
  LockingMock mock2(log, logMutex);
  mock1.Start();
  mock2.Start();
  mock1.Join();
  mock2.Join();
  ASSERT_EQ("started", log[0]);
  ASSERT_EQ("ended", log[1]);
  ASSERT_EQ("started", log[2]);
  ASSERT_EQ("ended", log[3]);
}
