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

#include "DefaultTimer.h"

using AdblockPlus::DefaultTimer;

DefaultTimer::DefaultTimer()
  : shouldThreadStop(false)
{
  m_thread = std::thread([this]
  {
    ThreadFunc();
  });
}

DefaultTimer::~DefaultTimer()
{
  {
    std::lock_guard<std::mutex> lock(mutex);
    shouldThreadStop = true;
  }
  conditionVariable.notify_one();
  if (m_thread.joinable())
    m_thread.join();
}

void DefaultTimer::SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback)
{
  if (!timerCallback)
    return;
  {
    std::lock_guard<std::mutex> lock(mutex);
    TimerUnit timer = { std::chrono::steady_clock::now() + timeout, timerCallback };
    timers.push(timer);
  }
  conditionVariable.notify_one();
}

void DefaultTimer::ThreadFunc()
{
  while (true)
  {
    std::unique_lock<std::mutex> lock(mutex);
    if (timers.empty())
    {
      conditionVariable.wait(lock, [this]()->bool
      {
        return shouldThreadStop || !timers.empty();
      });
    }
    else
    {
      auto fireAt = timers.top().fireAt;
      conditionVariable.wait_until(lock, fireAt);
    }
    // execute all expired timers and remove them
    while (!shouldThreadStop && !timers.empty() && timers.top().fireAt <= std::chrono::steady_clock::now())
    {
      auto callback = timers.top().callback;
      timers.pop();
      // allow to put new timers while this timer is being processed
      lock.unlock();
      try
      {
        callback();
      }
      catch (...)
      {
        // do nothing, but the thread will be alive.
      }
      lock.lock();
    }
    if (shouldThreadStop)
      return;
  }
}
