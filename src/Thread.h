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

#ifndef ADBLOCKPLUS_THREAD_H
#define ADBLOCKPLUS_THREAD_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AdblockPlus
{
  class Sync
  {
  public:
    Sync()
      : isSet(false)
    {
    }
    void Wait()
    {
      std::unique_lock<std::mutex> lock(mutex);
      if (!isSet)
        cv.wait(lock);
    }

    template<class R = typename std::chrono::seconds::rep,
             class P = std::ratio<1>>
    bool WaitFor(const std::chrono::duration<R, P>& duration = std::chrono::seconds(20))
    {
      std::unique_lock<std::mutex> lock(mutex);
      if (!isSet)
        return cv.wait_for(lock, duration) == std::cv_status::no_timeout;
      return true;
    }
    void Set(const std::string& err = std::string())
    {
      {
        std::unique_lock<std::mutex> lock(mutex);
        isSet = true;
        error = err;
      }
      cv.notify_all();
    }
    std::string GetError()
    {
      std::unique_lock<std::mutex> lock(mutex);
      return error;
    }
  private:
    std::mutex mutex;
    std::condition_variable cv;
    bool isSet;
    std::string error;
  };

  void Sleep(int millis);
}

#endif
