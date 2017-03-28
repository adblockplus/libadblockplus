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

#ifndef ADBLOCK_PLUS_TIMER_H
#define ADBLOCK_PLUS_TIMER_H

#include <functional>
#include <chrono>
#include <memory>

namespace AdblockPlus
{
  /**
   * Timer manager interface.
   */
  struct ITimer
  {
    /**
    * Callback type invoked after elapsing of timer timeout.
    */
    typedef std::function<void()> TimerCallback;
    virtual ~ITimer() {};

    /**
     * Sets a timer.
     * @param timeout A timer callback will be called after that interval.
     * @param timeCallback The callback which is called after timeout.
     */
    virtual void SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback) = 0;
  };

  /**
   * Unique smart pointer to an instance of `ITimer` implementation.
   */
  typedef std::unique_ptr<ITimer> TimerPtr;
}

#endif
