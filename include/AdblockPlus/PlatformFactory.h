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

#ifndef ADBLOCK_PLUS_PLATFORM_FACTORY_H
#define ADBLOCK_PLUS_PLATFORM_FACTORY_H

#include <AdblockPlus/Platform.h>

namespace AdblockPlus
{
  class PlatformFactory
  {
  public:
    /**
     * Various subsystems that can be replaced by your own implementation. In this case it is
     * important to remember that if your implementation uses a multi-threaded model, `Platform`
     * instance should be destroyed only after all threads have finished.
     */
    struct CreationParameters
    {
      LogSystemPtr logSystem;
      TimerPtr timer;
      WebRequestPtr webRequest;
      FileSystemPtr fileSystem;
      std::unique_ptr<IResourceReader> resourceReader;
      /**
       * Optional base path for file system. Used only if fileSystem is not provided.
       */
      std::string basePath;
    };

    /**
     * @param parameters platform configuration. If some fields are not set, default implementation
     *                   will be used.
     */
    static std::unique_ptr<Platform>
    CreatePlatform(CreationParameters&& parameters = CreationParameters());
  };
}

#endif // ADBLOCK_PLUS_PLATFORM_FACTORY_H
