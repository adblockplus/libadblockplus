#include <map>
#include <AdblockPlus.h>
#include "WebRequestJsObject.h"
#include "Thread.h"

namespace
{
  std::string fromV8String(v8::Handle<v8::Value> value)
  {
    v8::String::Utf8Value stringValue(value);
    if (stringValue.length())
      return std::string(*stringValue, stringValue.length());
    else
      return std::string();
  }

  v8::Local<v8::String> toV8String(const std::string& str)
  {
    return v8::String::New(str.c_str(), str.length());
  }

  class WebRequestThread : public AdblockPlus::Thread
  {
  public:
    WebRequestThread(const v8::Arguments& arguments)
        : isolate(v8::Isolate::GetCurrent()),
          context(v8::Persistent<v8::Context>::New(isolate, v8::Context::GetCurrent())),
          thisPtr(v8::Persistent<v8::Object>::New(isolate, arguments.Holder())),
          url(fromV8String(arguments[0]))
    {
      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;

      if (!url.length())
        throw std::runtime_error("Invalid string passed as first argument to GET");

      {
        const v8::Local<v8::Value> value = arguments[1];
        if (!value->IsObject())
          throw std::runtime_error("Second argument to GET must be an object");
        const v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(value);
        const v8::Local<v8::Array> properties = object->GetOwnPropertyNames();
        for (unsigned i = 0; i < properties->Length(); i++)
        {
          const v8::Local<v8::Value> property = properties->Get(i);
          std::string header = fromV8String(property);
          std::string headerValue = fromV8String(object->Get(property));
          if (header.length() && headerValue.length())
            headers.push_back(std::pair<std::string, std::string>(header, headerValue));
        }
      }

      const v8::Local<v8::Value> callbackValue = arguments[2];
      if (!callbackValue->IsFunction())
        throw std::runtime_error("Third argument to GET must be a function");
      callback = v8::Persistent<v8::Function>::New(isolate,
          v8::Local<v8::Function>::Cast(callbackValue));

      const v8::Local<const v8::External> external =
          v8::Local<const v8::External>::Cast(arguments.Data());
      webRequest = static_cast<AdblockPlus::WebRequest* const>(external->Value());
    }

    ~WebRequestThread()
    {
      context.Dispose(isolate);
      thisPtr.Dispose(isolate);
      callback.Dispose(isolate);
    }

    void Run()
    {
      AdblockPlus::ServerResponse result = webRequest->GET(url, headers);

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Local<v8::Object> resultObject = v8::Object::New();
      resultObject->Set(v8::String::New("status"), v8::Number::New(result.status));
      resultObject->Set(v8::String::New("responseStatus"), v8::Integer::New(result.responseStatus));
      resultObject->Set(v8::String::New("responseText"), toV8String(result.responseText));

      v8::Local<v8::Object> headersObject = v8::Object::New();
      for (AdblockPlus::HeaderList::iterator it = result.responseHeaders.begin();
        it != result.responseHeaders.end(); ++it)
      {
        headersObject->Set(toV8String(it->first), toV8String(it->second));
      }
      resultObject->Set(v8::String::New("responseHeaders"), headersObject);

      v8::Local<v8::Value> resultValue = resultObject;
      callback->Call(thisPtr, 1, &resultValue);
      delete this;
    }

  private:
    v8::Isolate* const isolate;
    v8::Persistent<v8::Context> context;
    v8::Persistent<v8::Object> thisPtr;
    std::string url;
    AdblockPlus::HeaderList headers;
    v8::Persistent<v8::Function> callback;
    AdblockPlus::WebRequest* webRequest;
  };

  v8::Handle<v8::Value> GETCallback(const v8::Arguments& arguments)
  {
    WebRequestThread* thread;
    try
    {
      if (arguments.Length() != 3u)
        throw std::runtime_error("GET requires exactly 3 arguments");
      thread = new WebRequestThread(arguments);
    }
    catch (const std::exception& e)
    {
      return v8::ThrowException(v8::String::New(e.what()));
    }
    thread->Start();
    return v8::Undefined();
  }
}

v8::Handle<v8::ObjectTemplate> AdblockPlus::WebRequestJsObject::Create(
  AdblockPlus::WebRequest& webRequest)
{
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> request = v8::ObjectTemplate::New();
  const v8::Handle<v8::FunctionTemplate> getFunction =
    v8::FunctionTemplate::New(::GETCallback,
                              v8::External::New(&webRequest));
  request->Set(v8::String::New("GET"), getFunction);
  return handleScope.Close(request);
}
