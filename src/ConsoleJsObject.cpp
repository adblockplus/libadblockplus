#include <AdblockPlus/ErrorCallback.h>
#include <sstream>

#include "ConsoleJsObject.h"

namespace
{
  v8::Handle<v8::Value> ErrorCallback(const v8::Arguments& arguments)
  {
    std::stringstream message;
    const v8::HandleScope handleScope;
    for (int i = 0; i < arguments.Length(); i++)
    {
      const v8::Handle<v8::Value> argument = arguments[i];
      const v8::String::Utf8Value value(argument);
      message << *value;
    }
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    AdblockPlus::ErrorCallback* const errorCallback =
      static_cast<AdblockPlus::ErrorCallback*>(external->Value());
    (*errorCallback)(message.str());
    return v8::Undefined();
  }

  v8::Handle<v8::Value> TraceCallback(const v8::Arguments& arguments)
  {
    return v8::Undefined();
  }
}

v8::Handle<v8::ObjectTemplate> AdblockPlus::ConsoleJsObject::Create(
  AdblockPlus::ErrorCallback& errorCallback)
{
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> console = v8::ObjectTemplate::New();
  const v8::Handle<v8::FunctionTemplate> errorFunction =
    v8::FunctionTemplate::New(::ErrorCallback,
                              v8::External::New(&errorCallback));
  console->Set(v8::String::New("error"), errorFunction);
  const v8::Handle<v8::FunctionTemplate> traceFunction =
    v8::FunctionTemplate::New(TraceCallback);
  console->Set(v8::String::New("trace"), traceFunction);
  return handleScope.Close(console);
}
