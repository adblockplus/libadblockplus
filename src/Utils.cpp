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

#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#endif

#include "Utils.h"

using namespace AdblockPlus;

void Utils::CheckTryCatch(v8::Isolate* isolate, const v8::TryCatch& tryCatch)
{
  if (tryCatch.HasCaught())
    throw AdblockPlus::JsError(isolate, tryCatch.Exception(), tryCatch.Message());
}

std::string Utils::FromV8String(v8::Isolate* isolate, const v8::Local<v8::Value>& value)
{
  v8::String::Utf8Value stringValue(isolate, value);
  if (stringValue.length())
    return std::string(*stringValue, stringValue.length());
  else
    return std::string();
}

StringBuffer Utils::StringBufferFromV8String(v8::Isolate* isolate, const v8::Local<v8::Value>& value)
{
  v8::String::Utf8Value stringValue(isolate, value);
  if (stringValue.length())
    return IFileSystem::IOBuffer(*stringValue, *stringValue + stringValue.length());
  else
    return IFileSystem::IOBuffer();
}

v8::MaybeLocal<v8::String> Utils::ToV8String(v8::Isolate* isolate, const std::string& str)
{
  return v8::String::NewFromUtf8(isolate, str.c_str(),
    v8::NewStringType::kNormal, str.length());
}

v8::MaybeLocal<v8::String> Utils::StringBufferToV8String(v8::Isolate* isolate, const StringBuffer& str)
{
  return v8::String::NewFromUtf8(isolate,
    reinterpret_cast<const char*>(str.data()),
    v8::NewStringType::kNormal, str.size());
}

void Utils::ThrowExceptionInJS(v8::Isolate* isolate, const std::string& str)
{
  auto maybe = Utils::ToV8String(isolate, str);
  if (maybe.IsEmpty())
  {
    isolate->ThrowException(
      Utils::ToV8String(isolate, "Unknown Exception").ToLocalChecked());
  }
  else
    isolate->ThrowException(maybe.ToLocalChecked());
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

