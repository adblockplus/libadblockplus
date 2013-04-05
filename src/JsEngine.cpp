#include <AdblockPlus.h>
#include <sstream>

#include "GlobalJsObject.h"

namespace
{
  v8::Handle<v8::Context> CreateContext(
    AdblockPlus::ErrorCallback& errorCallback)
  {
    const v8::Locker locker(v8::Isolate::GetCurrent());
    const v8::HandleScope handleScope;
    const v8::Handle<v8::ObjectTemplate> global =
      AdblockPlus::GlobalJsObject::Create(errorCallback);
    return v8::Context::New(0, global);
  }

  v8::Handle<v8::Script> CompileScript(const char* source, const char* filename)
  {
    const v8::Handle<v8::String> v8Source = v8::String::New(source);
    if (filename && filename[0])
    {
      const v8::Handle<v8::String> v8Filename = v8::String::New(filename);
      return v8::Script::Compile(v8Source, v8Filename);
    }
    else
      return v8::Script::Compile(v8Source);
  }

  void CheckTryCatch(const v8::TryCatch& tryCatch)
  {
    if (tryCatch.HasCaught())
      throw AdblockPlus::JsError(tryCatch.Exception(), tryCatch.Message());
  }

  std::string Slurp(std::istream& stream)
  {
    std::stringstream content;
    content << stream.rdbuf();
    return content.str();
  }

  std::string ExceptionToString(const v8::Handle<v8::Value> exception,
      const v8::Handle<v8::Message> message)
  {
    std::stringstream error;
    error << *v8::String::Utf8Value(exception);
    if (!message.IsEmpty())
    {
      error << " at ";
      error << *v8::String::Utf8Value(message->GetScriptResourceName());
      error << ":";
      error << message->GetLineNumber();
    }
    return error.str();
  }
}

AdblockPlus::JsError::JsError(const v8::Handle<v8::Value> exception,
    const v8::Handle<v8::Message> message)
  : std::runtime_error(ExceptionToString(exception, message))
{
}

AdblockPlus::JsEngine::JsEngine(const FileReader* const fileReader,
                                ErrorCallback* const errorCallback)
  : fileReader(fileReader), context(CreateContext(*errorCallback))
{
}

void AdblockPlus::JsEngine::Evaluate(const char* source, const char* filename)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  const v8::HandleScope handleScope;
  const v8::Context::Scope contextScope(context);
  const v8::TryCatch tryCatch;
  const v8::Handle<v8::Script> script = CompileScript(source, filename);
  CheckTryCatch(tryCatch);
  script->Run();
  CheckTryCatch(tryCatch);
}

void AdblockPlus::JsEngine::Evaluate(const std::string& source,
    const std::string& filename)
{
  Evaluate(source.c_str(), filename.c_str());
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
  const v8::Locker locker(v8::Isolate::GetCurrent());
  const v8::HandleScope handleScope;
  const v8::Context::Scope contextScope(context);
  const v8::Local<v8::Object> global = context->Global();
  const v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(
    global->Get(v8::String::New(functionName.c_str())));
  const v8::TryCatch tryCatch;
  const v8::Local<v8::Value> result = function->Call(function, 0, 0);
  CheckTryCatch(tryCatch);
  const v8::String::AsciiValue ascii(result);
  return *ascii;
}

std::string AdblockPlus::JsEngine::GetVariable(const std::string& name)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  const v8::HandleScope handleScope;
  const v8::Context::Scope contextScope(context);
  const v8::Local<v8::Object> global = context->Global();
  const v8::Local<v8::Value> value = global->Get(v8::String::New(name.c_str()));
  if (value->IsUndefined())
    return "";
  const v8::String::AsciiValue ascii(value);
  return *ascii;
}

void AdblockPlus::JsEngine::Gc()
{
  while (!v8::V8::IdleNotification());
}
