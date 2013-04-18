#include <map>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>
#include "WebRequestJsObject.h"
#include "Thread.h"

namespace
{
  class WebRequestThread : public AdblockPlus::Thread
  {
  public:
    WebRequestThread(AdblockPlus::JsEngine& jsEngine, AdblockPlus::JsValueList& arguments)
        : jsEngine(jsEngine), url(arguments[0]->AsString())
    {
      if (!url.length())
        throw std::runtime_error("Invalid string passed as first argument to GET");

      {
        AdblockPlus::JsValuePtr headersObj = arguments[1];
        if (!headersObj->IsObject())
          throw std::runtime_error("Second argument to GET must be an object");

        std::vector<std::string> properties = headersObj->GetOwnPropertyNames();
        for (std::vector<std::string>::iterator it = properties.begin();
            it != properties.end(); ++it)
        {
          std::string header = *it;
          std::string headerValue = headersObj->GetProperty(header)->AsString();
          if (header.length() && headerValue.length())
            headers.push_back(std::pair<std::string, std::string>(header, headerValue));
        }
      }

      callback = arguments[2];
      if (!callback->IsFunction())
        throw std::runtime_error("Third argument to GET must be a function");
    }

    void Run()
    {
      AdblockPlus::ServerResponse result = jsEngine.GetWebRequest().GET(url, headers);

      AdblockPlus::JsEngine::Context context(jsEngine);

      AdblockPlus::JsValuePtr resultObject = jsEngine.NewObject();
      resultObject->SetProperty("status", result.status);
      resultObject->SetProperty("responseStatus", result.responseStatus);
      resultObject->SetProperty("responseText", result.responseText);

      AdblockPlus::JsValuePtr headersObject = jsEngine.NewObject();
      for (AdblockPlus::HeaderList::iterator it = result.responseHeaders.begin();
        it != result.responseHeaders.end(); ++it)
      {
        headersObject->SetProperty(it->first, it->second);
      }
      resultObject->SetProperty("responseHeaders", headersObject);

      AdblockPlus::JsValueList params;
      params.push_back(resultObject);
      callback->Call(params);
      delete this;
    }

  private:
    AdblockPlus::JsEngine& jsEngine;
    std::string url;
    AdblockPlus::HeaderList headers;
    AdblockPlus::JsValuePtr callback;
  };

  v8::Handle<v8::Value> GETCallback(const v8::Arguments& arguments)
  {
    WebRequestThread* thread;
    try
    {
      AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
      AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);
      if (converted.size() != 3u)
        throw std::runtime_error("GET requires exactly 3 arguments");
      thread = new WebRequestThread(jsEngine, converted);
    }
    catch (const std::exception& e)
    {
      return v8::ThrowException(v8::String::New(e.what()));
    }
    thread->Start();
    return v8::Undefined();
  }
}

AdblockPlus::JsValuePtr AdblockPlus::WebRequestJsObject::Setup(
    AdblockPlus::JsEngine& jsEngine, AdblockPlus::JsValuePtr obj)
{
  obj->SetProperty("GET", jsEngine.NewCallback(::GETCallback));
  return obj;
}
