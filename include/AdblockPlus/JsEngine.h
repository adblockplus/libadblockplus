#ifndef ADBLOCKPLUS_JS_ENGINE_H
#define ADBLOCKPLUS_JS_ENGINE_H

#include <stdexcept>
#include <string>
#include <v8.h>

namespace AdblockPlus
{
  class ErrorCallback;
  class FileReader;
  class WebRequest;

  class JsError : public std::runtime_error
  {
  public:
    explicit JsError(const v8::Handle<v8::Value> exception,
        const v8::Handle<v8::Message> message);
  };

  class JsEngine
  {
  public:
    JsEngine(const FileReader* const fileReader,
             WebRequest* const webRequest,
             ErrorCallback* const errorCallback);
    std::string Evaluate(const std::string& source,
        const std::string& filename = "");
    void Load(const std::string& scriptPath);
    void Gc();

  private:
    const FileReader* const fileReader;
    v8::Persistent<v8::Context> context;
  };
}

#endif
