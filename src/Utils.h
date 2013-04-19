#ifndef ADBLOCK_PLUS_UTILS_H
#define ADBLOCK_PLUS_UTILS_H

#include <v8.h>

namespace AdblockPlus
{
  namespace Utils
  {
    std::string Slurp(std::ios& stream);
    std::string FromV8String(v8::Handle<v8::Value> value);
    v8::Local<v8::String> ToV8String(const std::string& str);
#ifdef _WIN32
    std::wstring ToUTF16String(const std::string& str, unsigned long length);
    std::string ToUTF8String(const std::wstring& str, unsigned long length);
    std::wstring CanonizeUrl(const std::wstring url);
#endif
  }
}

#endif
