/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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

#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#endif

#include "Utils.h"

using namespace AdblockPlus;

std::string Utils::Slurp(std::istream& stream)
{
  std::stringstream content;
  content << stream.rdbuf();
  return content.str();
}

std::string Utils::FromV8String(const v8::Handle<v8::Value>& value)
{
  v8::String::Utf8Value stringValue(value);
  if (stringValue.length())
    return std::string(*stringValue, stringValue.length());
  else
    return std::string();
}

v8::Local<v8::String> Utils::ToV8String(v8::Isolate* isolate, const std::string& str)
{
  return v8::String::NewFromUtf8(isolate, str.c_str(),
    v8::String::NewStringType::kNormalString, str.length());
}

void Utils::ThrowExceptionInJS(v8::Isolate* isolate, const std::string& str)
{
  isolate->ThrowException(Utils::ToV8String(isolate, str));
}

#ifdef _WIN32
std::wstring Utils::ToUtf16String(const std::string& str)
{
  size_t length = str.size();
  if (length == 0)
    return std::wstring();

  DWORD utf16StringLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, NULL, 0);
  if (utf16StringLength == 0)
    throw std::runtime_error("ToUTF16String failed. Can't determine the length of the buffer needed.");

  std::wstring utf16String(utf16StringLength, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, &utf16String[0], utf16StringLength);
  return utf16String;
}

std::string Utils::ToUtf8String(const std::wstring& str)
{
  size_t length = str.size();
  if (length == 0)
    return std::string();

  DWORD utf8StringLength = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), length, NULL, 0, 0, 0);
  if (utf8StringLength == 0)
    throw std::runtime_error("ToUTF8String failed. Can't determine the length of the buffer needed.");

  std::string utf8String(utf8StringLength, '\0');
  WideCharToMultiByte(CP_UTF8, 0, str.c_str(), length, &utf8String[0], utf8StringLength, 0, 0);
  return utf8String;
}

std::wstring Utils::CanonizeUrl(const std::wstring& url)
{
  HRESULT hr;

  std::wstring canonizedUrl;
  DWORD canonizedUrlLength = 2049; // de-facto limit of url length

  canonizedUrl.resize(canonizedUrlLength);
  hr = UrlCanonicalize(url.c_str(), &canonizedUrl[0], &canonizedUrlLength, 0);
  canonizedUrl.resize(canonizedUrlLength);
  if (FAILED(hr))
  {
    hr = UrlCanonicalize(url.c_str(), &canonizedUrl[0], &canonizedUrlLength, 0);
    if (FAILED(hr))
    {
      throw std::runtime_error("CanonizeUrl failed\n");
    }
  }
  return canonizedUrl;

}
#endif

