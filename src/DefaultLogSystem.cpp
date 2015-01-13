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

#include <iostream>
#include <AdblockPlus/DefaultLogSystem.h>

void AdblockPlus::DefaultLogSystem::operator()(AdblockPlus::LogSystem::LogLevel logLevel,
    const std::string& message, const std::string& source)
{
  switch (logLevel)
  {
    case LOG_LEVEL_TRACE:
      std::cerr << "Traceback:" << std::endl;
      break;
    case LOG_LEVEL_LOG:
      break;
    case LOG_LEVEL_INFO:
      std::cerr << "Info: ";
      break;
    case LOG_LEVEL_WARN:
      std::cerr << "Warning: ";
      break;
    case LOG_LEVEL_ERROR:
      std::cerr << "Error: ";
      break;
  }
  std::cerr << message;
  if (source.size())
    std::cerr << " at " << source;
  std::cerr << std::endl;
}
