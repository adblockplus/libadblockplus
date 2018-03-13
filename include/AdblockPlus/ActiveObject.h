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
#include <list>
#include <thread>
#include <functional>
#include "SynchronizedCollection.h"

namespace AdblockPlus
{
  /**
   * The implementation of active object pattern, simply put it sequentially
   * executes posted callable objects in a single background thread.
   * In the destructor it waits for the finishing of all already posted calls.
   */
  class ActiveObject {
  public:
    /**
     * A callable object type, in fact wrapping std::function.
     */
    typedef std::function<void()> Call;

    /**
     * Constructor, the background thread is started after finishing this call.
     */
    ActiveObject();

    /**
     * Destructor, it waits for finishing of all already posted calls.
     */
    ~ActiveObject();
    /**
     * Adds the `call`, which should be executed in the worker thread to the
     * end of the internally held list.
     * @param call object.
     */
    void Post(const Call& call);
    void Post(Call&& call);
  private:
    void ThreadFunc();
  private:
    bool isRunning;
    SynchronizedCollection<std::list<Call>> calls;
    std::thread thread;
  };
}