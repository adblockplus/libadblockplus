#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <string>
#include <v8.h>

class ErrorCallback;
class FileReader;

namespace AdblockPlus
{
  class JsEngine
  {
  public:
    JsEngine(const FileReader* const fileReader,
             ErrorCallback* const errorCallback);
    void Evaluate(const std::string& source);
    void Load(const std::string& scriptPath);
    std::string Call(const std::string& functionName);

  private:
    const FileReader* const fileReader;
    v8::Persistent<v8::Context> context;
  };
}

#endif

