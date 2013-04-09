#include <vector>
#include <stdexcept>

#include "GlobalJsObject.h"
#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  class TimeoutThread : public Thread
  {
  public:
    TimeoutThread(v8::Isolate* const isolate, const v8::Arguments& arguments)
      : isolate(isolate)
    {
      if (arguments.Length() < 2)
        throw std::runtime_error("setTimeout requires at least 2 parameters");

      const v8::Local<v8::Value> functionValue = arguments[0];
      if (!functionValue->IsFunction())
        throw std::runtime_error(
          "First argument to setTimeout must be a function");

      const v8::Local<v8::Value> delayValue = arguments[1];
      if (!delayValue->IsNumber())
        throw std::runtime_error(
          "Second argument to setTimeout must be a number");

      function = v8::Persistent<v8::Function>::New(
        isolate, v8::Local<v8::Function>::Cast(functionValue));
      delay = delayValue->ToNumber()->Value();
      for (int i = 2; i < arguments.Length(); i++)
      {
        const Value argument = Value::New(isolate, arguments[i]);
        functionArguments.push_back(argument);
      }
    }

    ~TimeoutThread()
    {
      function.Dispose(isolate);
      for (Values::iterator it = functionArguments.begin();
           it != functionArguments.end(); it++)
        it->Dispose(isolate);
    }

    void Run()
    {
      Sleep(delay);
      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      v8::Handle<v8::Value> argv = functionArguments.size() > 0 ? functionArguments[0] : v8::Handle<v8::Value>();
      function->Call(function, functionArguments.size(), &argv);
      delete this;
    }

  private:
    typedef v8::Persistent<v8::Value> Value;
    typedef std::vector<Value> Values;

    v8::Isolate* const isolate;
    v8::Persistent<v8::Function> function;
    int delay;
    Values functionArguments;
  };

  v8::Handle<v8::Value> SetTimeoutCallback(const v8::Arguments& arguments)
  {
    TimeoutThread* timeoutThread;
    try
    {
      timeoutThread = new TimeoutThread(v8::Isolate::GetCurrent(), arguments);
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
  ErrorCallback& errorCallback)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  const v8::Handle<v8::ObjectTemplate> console =
    AdblockPlus::ConsoleJsObject::Create(errorCallback);
  global->Set(v8::String::New("console"), console);
  const v8::Handle<v8::FunctionTemplate> setTimeoutFunction =
    v8::FunctionTemplate::New(SetTimeoutCallback);
  global->Set(v8::String::New("setTimeout"), setTimeoutFunction);
  return handleScope.Close(global);
}
