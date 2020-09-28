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

#include <AdblockPlus/AsyncExecutor.h>

using namespace AdblockPlus;

void AsyncExecutor::SyncThreads::SpawnThread(std::function<void(iterator)>&& task)
{
  std::lock_guard<std::mutex> lock(mutex);
  auto threadItertor = collection.emplace(collection.end());
  *threadItertor = /* A */ std::thread(std::move(task), threadItertor);
  // no need to notify here with the current usage of that class.
}

std::thread AsyncExecutor::SyncThreads::TakeOut(iterator pos)
{
  std::thread retValue;
  {
    std::lock_guard<std::mutex> lock(mutex);
    retValue = /* B */ std::move(*pos);
    collection.erase(pos);
  }
  conditionVar.notify_one();
  return retValue;
}

void AsyncExecutor::SyncThreads::WaitUtilEmpty()
{
  std::unique_lock<std::mutex> lock(mutex);
  conditionVar.wait(lock, [this]() -> bool {
    return collection.empty();
  });
}

AsyncExecutor::~AsyncExecutor()
{
  threads.WaitUtilEmpty();
}

void AsyncExecutor::Dispatch(const std::function<void()>& call)
{
  if (!call)
    return;
  threads.SpawnThread([this, call](SyncThreads::iterator threadIterator) {
    call();
    threadCollector.Post([this, threadIterator] {
      threads.TakeOut(threadIterator).join();
    });
  });
}