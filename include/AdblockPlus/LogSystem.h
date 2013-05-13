/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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

#include "tr1_memory.h"

namespace AdblockPlus
{
  class LogSystem
  {
  public:
    enum LogLevel {LOG_LEVEL_TRACE, LOG_LEVEL_LOG, LOG_LEVEL_INFO, LOG_LEVEL_WARN, LOG_LEVEL_ERROR};

    virtual ~LogSystem() {}
    virtual void operator()(LogLevel logLevel, const std::string& message,
          const std::string& source) = 0;
  };

  typedef std::tr1::shared_ptr<LogSystem> LogSystemPtr;
}

#endif
