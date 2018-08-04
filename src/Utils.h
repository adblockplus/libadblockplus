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

#ifndef ADBLOCK_PLUS_UTILS_H
#define ADBLOCK_PLUS_UTILS_H

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <vector>
#include <v8.h>

#include "AdblockPlus/JsValue.h"

#include "JsError.h"

namespace AdblockPlus
{
  namespace Utils
  {
    void CheckTryCatch(v8::Isolate* isolate, const v8::TryCatch& tryCatch);

    /*
     * Check for exception and then that a MaybeLocal<> isn't empty,
     * and throw a JsError if it is, otherwise return the Local<>
     * Call using the macro %CHECKED_MAYBE to get the location.
     */
    template<class T>
    v8::Local<T> CheckedToLocal(v8::Isolate* isolate,
      v8::MaybeLocal<T>&& value, const v8::TryCatch& tryCatch,
      const char* filename, int line)
    {
      CheckTryCatch(isolate, tryCatch);
      if (value.IsEmpty())
        throw AdblockPlus::JsError("Empty value at ", filename, line);
      return value.ToLocalChecked();
    }

#define CHECKED_TO_LOCAL(isolate, value, tryCatch)                      \
    AdblockPlus::Utils::CheckedToLocal(isolate, value, tryCatch, __FILE__, __LINE__)

    std::string FromV8String(v8::Isolate* isolate, const v8::Local<v8::Value>& value);
    StringBuffer StringBufferFromV8String(v8::Isolate* isolate, const v8::Local<v8::Value>& value);
    v8::Local<v8::String> ToV8String(v8::Isolate* isolate, const std::string& str);
    v8::Local<v8::String> StringBufferToV8String(v8::Isolate* isolate, const StringBuffer& bytes);
    void ThrowExceptionInJS(v8::Isolate* isolate, const std::string& str);

    // Code for templated function has to be in a header file, can't be in .cpp
    template<class T>
    T TrimString(const T& text)
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
