#ifndef ADBLOCKPLUS_JS_ENGINE_H
#define ADBLOCKPLUS_JS_ENGINE_H

#include <stdexcept>
#include <string>
#include <v8.h>

class ErrorCallback;
class FileReader;

namespace AdblockPlus
{
  class JsError : public std::runtime_error
  {
  public:
    explicit JsError(const v8::Handle<v8::Value> exception);
  };

  class JsEngine
  {
  public:
    JsEngine(const FileReader* const fileReader,
             ErrorCallback* const errorCallback);
    void Evaluate(const char* source, const char* filename = NULL);
    void Evaluate(const std::string& source,
        const std::string& filename = "");
    void Load(const std::string& scriptPath);
    std::string Call(const std::string& functionName);
    void Gc();

  private:
    const FileReader* const fileReader;
    v8::Persistent<v8::Context> context;
  };
}

#endif
