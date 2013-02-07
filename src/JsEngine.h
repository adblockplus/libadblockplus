#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <string>
#include <v8.h>

namespace AdblockPlus
{
  class JsEngine
  {
  public:
    JsEngine();
    void Evaluate(const std::string& source);
    std::string Call(const std::string& functionName);

  private:
    v8::Persistent<v8::Context> context;
  };
}

#endif
