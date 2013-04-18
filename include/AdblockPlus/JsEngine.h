#ifndef ADBLOCKPLUS_JS_ENGINE_H
#define ADBLOCKPLUS_JS_ENGINE_H

#include <stdexcept>
#include <string>
#include <v8.h>
#include <AdblockPlus/JsValue.h>

namespace AdblockPlus
{
  struct AppInfo;
  class FileSystem;
  class WebRequest;
  class ErrorCallback;

  class JsError : public std::runtime_error
  {
  public:
    explicit JsError(const v8::Handle<v8::Value> exception,
        const v8::Handle<v8::Message> message);
  };

  class JsEngine
  {
    friend class JsValue;

  public:
    JsEngine(const AppInfo& appInfo,
             FileSystem* const fileReader,
             WebRequest* const webRequest,
             ErrorCallback* const errorCallback);
    JsValuePtr Evaluate(const std::string& source,
        const std::string& filename = "");
    void Load(const std::string& scriptPath);
    void Gc();
    JsValuePtr NewValue(const std::string& val);
    JsValuePtr NewValue(int64_t val);
    JsValuePtr NewValue(bool val);
    inline JsValuePtr NewValue(const char* val)
    {
      return NewValue(std::string(val));
    }
    inline JsValuePtr NewValue(int val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
    JsValuePtr NewObject();
    JsValuePtr NewCallback(v8::InvocationCallback callback);
    static JsEngine& FromArguments(const v8::Arguments& arguments);
    JsValueList ConvertArguments(const v8::Arguments& arguments);

    inline FileSystem& GetFileSystem()
    {
      return fileSystem;
    }
    inline WebRequest& GetWebRequest()
    {
      return webRequest;
    }
    inline ErrorCallback& GetErrorCallback()
    {
      return errorCallback;
    }

    class Context
    {
    public:
      Context(const JsEngine& jsEngine);
      virtual inline ~Context() {};

    private:
      const v8::Locker locker;
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope;
    };

  private:
    FileSystem& fileSystem;
    WebRequest& webRequest;
    ErrorCallback& errorCallback;
    v8::Isolate* isolate;
    v8::Persistent<v8::Context> context;
  };
}

#endif
