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

#ifndef ADBLOCK_PLUS_APP_INFO_H
#define ADBLOCK_PLUS_APP_INFO_H

#include <string>

namespace AdblockPlus
{
  struct AppInfo
  {
    std::string id;
    std::string version;
    std::string name;
    std::string application;
    std::string applicationVersion;
    std::string locale;
    bool developmentBuild;

    AppInfo() : developmentBuild(false) {}
  };
}

#endif
