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

#ifndef ADBLOCK_PLUS_SCHEDULER_H
#define ADBLOCK_PLUS_SCHEDULER_H

namespace AdblockPlus
{
  /**
   * Task object which can be passed to `Scheduler` to be executed asynchronously.
   */
  typedef std::function<void()> SchedulerTask;

  /**
   * Scheduler object executing tasks asynchronously.
   */
  typedef std::function<void(const SchedulerTask&)> Scheduler;
}
#endif // ADBLOCK_PLUS_SCHEDULER_H