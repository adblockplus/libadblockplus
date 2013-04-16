#include <sstream>
#include <string>

#include "Utils.h"

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
