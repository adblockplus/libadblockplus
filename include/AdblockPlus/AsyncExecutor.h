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
 * along with Adblock Plus. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include "ActiveObject.h"

namespace AdblockPlus
{
  /**
   * Spawns a new thread for each task and waits for finishing of all spawned
   * threads in the destructor.
   */
  class AsyncExecutor
  {
    // This class not only serializes access to the list of threads but also
    // ensures that internals of std::thread are valid in another (collecting)
    // thread. The latter is about sentries protecting A and B because
    // otherwise it can happen that a worker thread has already finished the
    // call, passed info to the collector thread and the collector is trying
    // to get information from iterator (B) but the assignment (A) has not
    // happened yet.
    class SyncThreads
    {
      typedef std::list<std::thread> Threads;
    public:
      typedef Threads::iterator iterator;
      void SpawnThread(std::function<void(iterator)>&& task);
      std::thread TakeOut(iterator pos);
      void WaitUtilEmpty();
    protected:
      Threads collection;
      std::mutex mutex;
      std::condition_variable conditionVar;
    };
  public:
    /**
     * Destructor, it waits for finishing of all already dispatched tasks.
     */
    ~AsyncExecutor();

    /**
     * Creates a new thread in which the `call` will be executed.
     * @param call is a function object which is called within a worker thread,
     *        different from the caller thread. There is no effect if `call` is
     *        empty.
     */
    void Dispatch(const std::function<void()>& call);
  private:
    SyncThreads threads;
    ActiveObject threadCollector;
  };
}