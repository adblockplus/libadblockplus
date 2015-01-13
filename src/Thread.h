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

#ifndef ADBLOCKPLUS_THREAD_H
#define ADBLOCKPLUS_THREAD_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AdblockPlus
{
  void Sleep(const int millis);

  class Mutex
  {
  public:
#ifdef WIN32
    CRITICAL_SECTION nativeMutex;
#else
    pthread_mutex_t nativeMutex;
#endif

    Mutex();
    ~Mutex();
    void Lock();
    void Unlock();
  };

  class Lock
  {
  public:
    Lock(Mutex& mutex);
    ~Lock();

  private:
    Mutex& mutex;
  };

  class Thread
  {
  public:
    virtual ~Thread();
    virtual void Run() = 0;
    void Start();
    void Join();

  private:
#ifdef WIN32
    HANDLE nativeThread;
#else
    pthread_t nativeThread;
#endif
  };
}

#endif
