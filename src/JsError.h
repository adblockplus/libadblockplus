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

#ifndef ADBLOCK_PLUS_JS_ERROR_H
#define ADBLOCK_PLUS_JS_ERROR_H

#include <sstream>
#include <stdexcept>
#include <v8.h>

namespace AdblockPlus
{
  class JsError : public std::runtime_error
  {
  public:
    JsError(const v8::Handle<v8::Value>& exception,
            const v8::Handle<v8::Message>& message);
    static std::string ExceptionToString(const v8::Handle<v8::Value>& exception,
      const v8::Handle<v8::Message>& message);

  };
}

#endif
