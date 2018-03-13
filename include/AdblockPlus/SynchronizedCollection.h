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
#include <mutex>
#include <condition_variable>

namespace AdblockPlus
{
  /**
   * Wrapper around `Container` providing few generic methods which ensure
   * that the underlying container is accessed only by one thread at the same
   * time.
   */
  template<typename TContainer>
  class SynchronizedCollection
  {
  protected:
    typedef TContainer Container;
  public:
    /**
     * The `value_type` represents the type of stored values.
     */
    typedef typename Container::value_type value_type;

    /**
     * Adds `value` normally to the end.
     * @param value which is stored.
     */
    void push_back(const value_type& value)
    {
      {
        std::lock_guard<std::mutex> lock(mutex);
        collection.push_back(value);
      }
      conditionVar.notify_one();
    }
    void push_back(value_type&& value)
    {
      {
        std::lock_guard<std::mutex> lock(mutex);
        collection.push_back(std::move(value));
      }
      conditionVar.notify_one();
    }

    /**
     * Extracts the first stored element and returns it.
     * Pay attention that the call of this method blocks the execution until
     * there is at least one element added to the collection.
     */
    value_type pop_front()
    {
      std::unique_lock<std::mutex> lock(mutex);
      conditionVar.wait(lock, [this]()->bool
      {
        return !collection.empty();
      });
      value_type retValue = collection.front();
      collection.pop_front();
      return retValue;
    }
  protected:
    Container collection;
    std::mutex mutex;
    std::condition_variable conditionVar;
  };
}