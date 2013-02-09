#include <AdblockPlus.h>
#include <sstream>

namespace
{
  v8::Handle<v8::Value> ReportErrorCallback(const v8::Arguments& arguments)
  {
    if (arguments.Length() < 1)
      return v8::Undefined();
    const v8::HandleScope handleScope;
    const v8::Handle<v8::Value> argument = arguments[0];
    const v8::String::Utf8Value value(argument);
    const v8::Handle<v8::External> external =
      v8::Handle<v8::External>::Cast(arguments.Data());
    AdblockPlus::ErrorCallback* const errorCallback =
      static_cast<AdblockPlus::ErrorCallback*>(external->Value());
    (*errorCallback)(*value);
    return v8::Undefined();
  }

  v8::Handle<v8::Context> CreateContext(
    AdblockPlus::ErrorCallback* const errorCallback)
  {
    const v8::HandleScope handleScope;
    const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
    const v8::Handle<v8::FunctionTemplate> reportErrorTemplate =
      v8::FunctionTemplate::New(ReportErrorCallback,
                                v8::External::New(errorCallback));
    global->Set(v8::String::New("reportError"), reportErrorTemplate);
    const v8::Persistent<v8::Context> context = v8::Context::New(0, global);
    return context;
  }

  std::string Slurp(const std::istream& stream)
  {
    std::stringstream content;
    content << stream.rdbuf();
    return content.str();
  }
}

AdblockPlus::JsError::JsError(const std::string& message)
  : std::runtime_error(message)
{
}

AdblockPlus::JsEngine::JsEngine(const FileReader* const fileReader,
                                ErrorCallback* const errorCallback)
  : fileReader(fileReader), context(CreateContext(errorCallback))
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
  if (result.IsEmpty()) {
    const v8::Handle<v8::Value> exception = tryCatch.Exception();
    const v8::String::AsciiValue message(exception);
    throw JsError(*message);
  }
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
  if (result.IsEmpty()) {
    const v8::Handle<v8::Value> exception = tryCatch.Exception();
    const v8::String::AsciiValue message(exception);
    throw JsError(*message);
  }
  const v8::String::AsciiValue ascii(result);
  return *ascii;
}
