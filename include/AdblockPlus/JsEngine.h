#ifndef ADBLOCKPLUS_JS_ENGINE_H
#define ADBLOCKPLUS_JS_ENGINE_H

#include <stdexcept>
#include <string>
#include <v8.h>
#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/ErrorCallback.h>
#include <AdblockPlus/FileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class JsError : public std::runtime_error
  {
  public:
    JsError(const v8::Handle<v8::Value> exception,
            const v8::Handle<v8::Message> message);
  };

  typedef std::tr1::shared_ptr<JsEngine> JsEnginePtr;

  class JsEngine
  {
    friend class JsValue;

  public:
    JsEngine(const AppInfo& appInfo = AppInfo());
    JsValuePtr Evaluate(const std::string& source,
        const std::string& filename = "");
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

    FileSystemPtr GetFileSystem();
    void SetFileSystem(FileSystemPtr val);
    WebRequestPtr GetWebRequest();
    void SetWebRequest(WebRequestPtr val);
    ErrorCallbackPtr GetErrorCallback();
    void SetErrorCallback(ErrorCallbackPtr val);

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
    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
    ErrorCallbackPtr errorCallback;
    v8::Isolate* isolate;
    v8::Persistent<v8::Context> context;
  };
}

#endif
