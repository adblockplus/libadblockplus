#include <AdblockPlus.h>
#include <sstream>

#include "GlobalJsObject.h"
#include "Utils.h"

namespace
{
  v8::Handle<v8::Context> CreateContext(
    v8::Isolate* isolate,
    AdblockPlus::FileSystem& fileSystem,
    AdblockPlus::WebRequest& webRequest,
    AdblockPlus::ErrorCallback& errorCallback)
  {
    const v8::Locker locker(isolate);
    const v8::HandleScope handleScope;
    const v8::Handle<v8::ObjectTemplate> global =
      AdblockPlus::GlobalJsObject::Create(fileSystem, webRequest,
                                          errorCallback);
    return v8::Context::New(0, global);
  }

  v8::Handle<v8::Script> CompileScript(const std::string& source, const std::string& filename)
  {
    const v8::Handle<v8::String> v8Source = v8::String::New(source.c_str());
    if (filename.length())
    {
      const v8::Handle<v8::String> v8Filename = v8::String::New(filename.c_str());
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

AdblockPlus::JsEngine::JsEngine(FileSystem* const fileSystem,
                                WebRequest* const webRequest,
                                ErrorCallback* const errorCallback)
  : fileSystem(fileSystem), isolate(v8::Isolate::GetCurrent()),
    context(CreateContext(isolate, *fileSystem, *webRequest, *errorCallback))
{
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::Evaluate(const std::string& source,
    const std::string& filename)
{
  const Context context(*this);
  const v8::TryCatch tryCatch;
  const v8::Handle<v8::Script> script = CompileScript(source, filename);
  CheckTryCatch(tryCatch);
  v8::Local<v8::Value> result = script->Run();
  CheckTryCatch(tryCatch);
  return JsValuePtr(new JsValue(*this, result));
}

void AdblockPlus::JsEngine::Load(const std::string& scriptPath)
{
  const std::tr1::shared_ptr<std::istream> file = fileSystem->Read(scriptPath);
  if (!file || !*file)
    throw std::runtime_error("Unable to load script " + scriptPath);
  Evaluate(Utils::Slurp(*file));
}

void AdblockPlus::JsEngine::Gc()
{
  while (!v8::V8::IdleNotification());
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(const std::string& val)
{
  const Context context(*this);
  return JsValuePtr(new JsValue(*this, v8::String::New(val.c_str(), val.length())));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(int64_t val)
{
  const Context context(*this);
  return JsValuePtr(new JsValue(*this, v8::Integer::New(val)));
}

AdblockPlus::JsValuePtr AdblockPlus::JsEngine::NewValue(bool val)
{
  const Context context(*this);
  return JsValuePtr(new JsValue(*this, v8::Boolean::New(val)));
}

AdblockPlus::JsEngine::Context::Context(const JsEngine& jsEngine)
    : locker(jsEngine.isolate), handleScope(),
      contextScope(jsEngine.context)
{
}
