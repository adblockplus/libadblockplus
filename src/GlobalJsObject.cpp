#include <vector>
#include <stdexcept>

#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>

#include "AppInfoJsObject.h"
#include "ConsoleJsObject.h"
#include "FileSystemJsObject.h"
#include "GlobalJsObject.h"
#include "ConsoleJsObject.h"
#include "WebRequestJsObject.h"
#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  class TimeoutThread : public Thread
  {
  public:
    TimeoutThread(JsValueList& arguments)
    {
      if (arguments.size() < 2)
        throw std::runtime_error("setTimeout requires at least 2 parameters");

      if (!arguments[0]->IsFunction())
        throw std::runtime_error(
          "First argument to setTimeout must be a function");

      function = arguments[0];
      delay = arguments[1]->AsInt();
      for (size_t i = 2; i < arguments.size(); i++)
        functionArguments.push_back(arguments[i]);
    }

    void Run()
    {
      Sleep(delay);

      function->Call(functionArguments);
    }

  private:
    JsValuePtr function;
    int delay;
    JsValueList functionArguments;
  };

  v8::Handle<v8::Value> SetTimeoutCallback(const v8::Arguments& arguments)
  {
    TimeoutThread* timeoutThread;
    try
    {
      AdblockPlus::JsValueList converted =
          AdblockPlus::JsEngine::FromArguments(arguments)
          .ConvertArguments(arguments);
      timeoutThread = new TimeoutThread(converted);
    }
    catch (const std::exception& e)
    {
      return v8::ThrowException(v8::String::New(e.what()));
    }
    timeoutThread->Start();

    // We should actually return the timer ID here, which could be
    // used via clearTimeout(). But since we don't seem to need
    // clearTimeout(), we can save that for later.
    return v8::Undefined();
  }
}

v8::Handle<v8::ObjectTemplate> GlobalJsObject::Create(
  const AppInfo& appInfo, JsEngine& jsEngine)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  const v8::Handle<v8::FunctionTemplate> setTimeoutFunction =
    v8::FunctionTemplate::New(SetTimeoutCallback,
                              v8::External::New(&jsEngine));
  global->Set(v8::String::New("setTimeout"), setTimeoutFunction);
  const v8::Handle<v8::ObjectTemplate> fileSystemObject =
    FileSystemJsObject::Create(jsEngine);
  global->Set(v8::String::New("_fileSystem"), fileSystemObject);
  const v8::Handle<v8::ObjectTemplate> webRequestObject =
    WebRequestJsObject::Create(jsEngine);
  global->Set(v8::String::New("_webRequest"), webRequestObject);
  const v8::Handle<v8::ObjectTemplate> consoleObject =
    ConsoleJsObject::Create(jsEngine);
  global->Set(v8::String::New("console"), consoleObject);
  const v8::Handle<v8::ObjectTemplate> appInfoObject =
    AppInfoJsObject::Create(appInfo);
  global->Set(v8::String::New("_appInfo"), appInfoObject);
  return handleScope.Close(global);
}
