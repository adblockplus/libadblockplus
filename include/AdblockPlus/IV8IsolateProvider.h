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

#pragma once

#include <memory>

namespace v8
{
  class Isolate;
}

// #define UNBLOCK_UPDATE_PAST_DP_1347_ON_ANDROID

namespace AdblockPlus
{
  /**
   * Provides with isolate. The main aim of this iterface is to delegate a
   * proper initialization and deinitialization of v8::Isolate to an embedder.
   */
  struct IV8IsolateProvider
  {
    virtual ~IV8IsolateProvider()
    {
    }

    /**
     * Returns v8::Isolate. All subsequent calls of this method should return
     * the same pointer to v8::Isolate as the first call.
     */
    virtual v8::Isolate* Get() = 0;
  };

  using IV8IsolateProviderPtr =
#if !defined(UNBLOCK_UPDATE_PAST_DP_1347_ON_ANDROID)
      IV8IsolateProvider*;
#else
      // shared because it's copied within JsValue.
      std::shared_ptr<IV8IsolateProvider>;
#endif

}
