/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#ifndef ADBLOCK_PLUS_LOG_SYSTEM_H
#define ADBLOCK_PLUS_LOG_SYSTEM_H

#include <string>
#include <memory>

namespace AdblockPlus
{
  /**
   * Logging interface.
   */
  class LogSystem
  {
  public:
    /**
     * Log level.
     */
    enum LogLevel {LOG_LEVEL_TRACE, LOG_LEVEL_LOG, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR};

    virtual ~LogSystem() {}

    /**
     * Writes a log message.
     * @param logLevel Log level.
     * @param message Log message.
     * @param source Source of the message, e.g. file name and line.
     *        Ignored when empty.
     */
    virtual void operator()(LogLevel logLevel, const std::string& message,
          const std::string& source) = 0;
  };

  /**
   * Shared smart pointer to a `LogSystem` instance.
   */
  typedef std::shared_ptr<LogSystem> LogSystemPtr;
}

#endif
