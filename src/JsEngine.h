#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <string>
#include <v8.h>

class FileReader;

namespace AdblockPlus
{
  class JsEngine
  {
  public:
    FileReader* fileReader;

    JsEngine();
    void Evaluate(const std::string& source);
    void Load(const std::string& scriptPath);
    std::string Call(const std::string& functionName);

  private:
    v8::Persistent<v8::Context> context;
  };
}

#endif
