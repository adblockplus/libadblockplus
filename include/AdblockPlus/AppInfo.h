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

#ifndef ADBLOCK_PLUS_APP_INFO_H
#define ADBLOCK_PLUS_APP_INFO_H

#include <string>

namespace AdblockPlus
{
  /**
   * Information about the app using libadblockplus.
   */
  struct AppInfo
  {
    /**
     * Current version of the app, in
     * [Mozilla toolkit version format](https://developer.mozilla.org/en/docs/Toolkit_version_format).
     */
    std::string version;

    /**
     * Technical name of the app (not user visible).
     */
    std::string name;

    /**
     * Technical name of the platform the app is running on (not user visible).
     */
    std::string application;

    /**
     * Current version of the platform the app is running on.
     */
    std::string applicationVersion;

    /**
     * Locale to use, as a
     * [Mozilla locale code](https://wiki.mozilla.org/L10n:Locale_Codes).
     */
    std::string locale;

    /**
     * Whether the app is a development build, the default is `false`.
     */
    bool developmentBuild;

    AppInfo() : developmentBuild(false) {}
  };
}

#endif
