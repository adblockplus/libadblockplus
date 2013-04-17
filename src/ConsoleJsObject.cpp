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

v8::Handle<v8::ObjectTemplate> AdblockPlus::ConsoleJsObject::Create(
  AdblockPlus::JsEngine& jsEngine)
{
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> console = v8::ObjectTemplate::New();
  const v8::Handle<v8::FunctionTemplate> errorFunction =
    v8::FunctionTemplate::New(::ErrorCallback,
                              v8::External::New(&jsEngine));
  console->Set(v8::String::New("error"), errorFunction);
  const v8::Handle<v8::FunctionTemplate> traceFunction =
    v8::FunctionTemplate::New(TraceCallback);
  console->Set(v8::String::New("trace"), traceFunction);
  return handleScope.Close(console);
}
