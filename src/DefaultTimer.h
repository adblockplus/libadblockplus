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

#ifndef ADBLOCK_PLUS_DEFAULT_TIMER_H
#define ADBLOCK_PLUS_DEFAULT_TIMER_H

#include <AdblockPlus/ITimer.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace AdblockPlus
{
  class DefaultTimer : public ITimer
  {
    struct TimerUnit
    {
      std::chrono::steady_clock::time_point fireAt;
      TimerCallback callback;
    };
    struct TimerUnitComparator
    {
      bool operator()(const TimerUnit& t1, const TimerUnit& t2) const
      {
        // pay attention 2 < 1 because we need the smallest time at the top.
        return t2.fireAt < t1.fireAt;
      }
    };
    typedef std::priority_queue<TimerUnit, std::vector<TimerUnit>, TimerUnitComparator> TimerUnits;
  public:
    DefaultTimer();
    ~DefaultTimer();
    void SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback) override;
  private:
    void ThreadFunc();
  private:
    std::mutex mutex;
    std::condition_variable conditionVariable;
    TimerUnits timers;
    bool shouldThreadStop;
    std::thread m_thread;
  };
}

#endif