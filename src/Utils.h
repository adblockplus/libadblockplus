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

#ifndef ADBLOCK_PLUS_UTILS_H
#define ADBLOCK_PLUS_UTILS_H

#include <algorithm>
#include <cctype>
#include <functional>
#include <istream>
#include <string>
#include <v8.h>

namespace AdblockPlus
{
  namespace Utils
  {
    std::string Slurp(std::istream& stream);
    std::string FromV8String(v8::Handle<v8::Value> value);
    v8::Local<v8::String> ToV8String(const std::string& str);

    // Code for templated function has to be in a header file, can't be in .cpp
    template<class T>
    T TrimString(T text)
    {
      // Via http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
      T trimmed(text);
      trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
      trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());
      return trimmed;
    }
#ifdef _WIN32
    std::wstring ToUtf16String(const std::string& str);
    std::string ToUtf8String(const std::wstring& str);
    std::wstring CanonizeUrl(const std::wstring& url);
#endif
  }
}

#endif
