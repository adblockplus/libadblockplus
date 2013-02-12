#include <AdblockPlus.h>
#include <sstream>

#include "JsConsole.h"

namespace
{
  v8::Handle<v8::Context> CreateContext(
    AdblockPlus::ErrorCallback& errorCallback)
  {
    const v8::HandleScope handleScope;
    const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
    global->Set(v8::String::New("console"), JsConsole::Create(errorCallback));
    const v8::Persistent<v8::Context> context = v8::Context::New(0, global);
    return context;
  }

  std::string Slurp(std::istream& stream)
  {
    std::stringstream content;
    content << stream.rdbuf();
    return content.str();
  }
}

AdblockPlus::JsError::JsError(const v8::Handle<v8::Value> exception)
  : std::runtime_error(*v8::String::AsciiValue(exception))
{
}

AdblockPlus::JsEngine::JsEngine(const FileReader* const fileReader,
                                ErrorCallback* const errorCallback)
  : fileReader(fileReader), context(CreateContext(*errorCallback))
{
}

void AdblockPlus::JsEngine::Evaluate(const std::string& source)
{
  const v8::HandleScope handleScope;
  const v8::Context::Scope contextScope(context);
  const v8::Handle<v8::String> v8Source = v8::String::New(source.c_str());
  const v8::Handle<v8::Script> script = v8::Script::Compile(v8Source);
  const v8::TryCatch tryCatch;
  const v8::Handle<v8::Value> result = script->Run();
  if (result.IsEmpty())
    throw JsError(tryCatch.Exception());
}

void AdblockPlus::JsEngine::Load(const std::string& scriptPath)
{
  const std::auto_ptr<std::istream> file = fileReader->Read(scriptPath);
  if (!*file)
    throw std::runtime_error("Unable to load script " + scriptPath);
  Evaluate(Slurp(*file));
}

std::string AdblockPlus::JsEngine::Call(const std::string& functionName)
{
  const v8::HandleScope handleScope;
  const v8::Context::Scope contextScope(context);
  const v8::Local<v8::Object> global = context->Global();
  const v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(
    global->Get(v8::String::New(functionName.c_str())));
  const v8::TryCatch tryCatch;
  const v8::Local<v8::Value> result = function->Call(function, 0, 0);
  if (result.IsEmpty())
    throw JsError(tryCatch.Exception());
  const v8::String::AsciiValue ascii(result);
  return *ascii;
}
