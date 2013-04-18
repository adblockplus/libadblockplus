#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/ErrorCallback.h>
#include <sstream>

#include "ConsoleJsObject.h"

namespace
{
  v8::Handle<v8::Value> ErrorCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    const AdblockPlus::JsEngine::Context context(jsEngine);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    std::stringstream message;
    for (size_t i = 0; i < converted.size(); i++)
      message << converted[i]->AsString();

    (jsEngine.GetErrorCallback())(message.str());
    return v8::Undefined();
  }

  v8::Handle<v8::Value> TraceCallback(const v8::Arguments& arguments)
  {
    return v8::Undefined();
  }
}

AdblockPlus::JsValuePtr AdblockPlus::ConsoleJsObject::Setup(
    AdblockPlus::JsEngine& jsEngine, AdblockPlus::JsValuePtr obj)
{
  obj->SetProperty("error", jsEngine.NewCallback(::ErrorCallback));
  obj->SetProperty("trace", jsEngine.NewCallback(::TraceCallback));
  return obj;
}
