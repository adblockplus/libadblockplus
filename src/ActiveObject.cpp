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
#include <AdblockPlus/ActiveObject.h>

using namespace AdblockPlus;

ActiveObject::ActiveObject() : isRunning(true)
{
  thread = std::thread([this] {
    ThreadFunc();
  });
}

ActiveObject::~ActiveObject()
{
  Post([this] {
    isRunning = false;
  });
  thread.join();
}

void ActiveObject::Post(const Call& call)
{
  if (!call)
    return;
  calls.push_back(call);
}

void ActiveObject::Post(Call&& call)
{
  if (!call)
    return;
  calls.push_back(std::move(call));
}

void ActiveObject::ThreadFunc()
{
  while (isRunning)
  {
    Call call = calls.pop_front();
    try
    {
      call();
    }
    catch (...)
    {
      // do nothing, but the thread will be alive.
    }
  }
}
