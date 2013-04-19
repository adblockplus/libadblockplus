#include <sstream>
#include <string>

#include "Utils.h"
#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>

#include <algorithm>
#include <cctype>
#include <functional>

#endif

using namespace AdblockPlus;

std::string Utils::Slurp(std::ios& stream)
{
  std::stringstream content;
  content << stream.rdbuf();
  return content.str();
}

std::string Utils::FromV8String(v8::Handle<v8::Value> value)
{
  v8::String::Utf8Value stringValue(value);
  if (stringValue.length())
    return std::string(*stringValue, stringValue.length());
  else
    return std::string();
}

v8::Local<v8::String> Utils::ToV8String(const std::string& str)
{
  return v8::String::New(str.c_str(), str.length());
}

#ifdef _WIN32
std::wstring Utils::ToUTF16String(const std::string& str, unsigned long length)
{
  if (length == 0)
    return std::wstring();
  DWORD utf16StringLength = 0;
  std::wstring utf16String;
  utf16StringLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, &utf16String[0], utf16StringLength);
  if (utf16StringLength > 0)
  {
    utf16String.resize(utf16StringLength + 1);
    utf16StringLength = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), length, &utf16String[0], utf16StringLength);
    utf16String[utf16StringLength] = L'\0';  
    return utf16String;
  }
  std::runtime_error("ToUTF16String failed. Can't determine the length of the buffer needed\n");
  return 0;
}



std::string Utils::ToUTF8String(const std::wstring& str, unsigned long length)
{
  if (length == 0)
    return std::string();

  DWORD utf8StringLength = 0;
  std::string utf8String;
  utf8StringLength = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), length, &utf8String[0], utf8StringLength, 0, 0);
  if (utf8StringLength > 0)
  {
    utf8String.resize(utf8StringLength + 1);
    utf8StringLength = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), length, &utf8String[0], utf8StringLength, 0, 0);
    utf8String[utf8StringLength] = L'\0';  

    return utf8String;
  }
  std::runtime_error("ToUTF8String failed. Can't determine the length of the buffer needed\n");
  return 0;
}

std::wstring Utils::CanonizeUrl(std::wstring url)
{
  HRESULT hr;

  std::wstring canonizedUrl;
  DWORD canonizedUrlLength = 2049; // de-facto limit of url length

  canonizedUrl.resize(canonizedUrlLength);  
  hr = UrlCanonicalize(url.c_str(), &canonizedUrl[0], &canonizedUrlLength, 0);
  if (FAILED(hr))
  {
    // The URL was too long. Let's try again with an increased buffer
    canonizedUrl.resize(canonizedUrlLength + 1);
    hr = UrlCanonicalize(url.c_str(), &canonizedUrl[0], &canonizedUrlLength, 0);
    if (FAILED(hr))
    {
      std::runtime_error("CanonizeUrl failed\n");
    }
  }
  return canonizedUrl;

}

std::wstring Utils::TrimString(std::wstring text)
{
  // Via http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
  std::wstring trimmed(text);
  trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());
  return trimmed;
}

#endif
