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
    const v8::String::Utf8Value message(argument);
    const v8::Handle<v8::External> external =
      v8::Handle<v8::External>::Cast(arguments.Data());
    AdblockPlus::ErrorCallback* const errorCallback =
      static_cast<AdblockPlus::ErrorCallback*>(external->Value());
    (*errorCallback)(*message);
    return v8::Undefined();
  }

  std::string Slurp(const AdblockPlus::FileReader& fileReader,
                    const std::string& path)
  {
    const std::auto_ptr<std::istream> file = fileReader.Read(path);
    if (!*file)
      throw std::runtime_error("Unable to load file " + path);
    std::stringstream content;
    content << file->rdbuf();
    return content.str();
  }

  v8::Handle<v8::Value> LoadCallback(const v8::Arguments& arguments)
  {
    if (arguments.Length() < 1)
      return v8::Undefined();
    const v8::HandleScope handleScope;
    const v8::Handle<v8::Value> argument = arguments[0];
    const v8::String::Utf8Value path(argument);
    const v8::Handle<v8::External> external =
      v8::Handle<v8::External>::Cast(arguments.Data());
    AdblockPlus::FileReader* const fileReader =
      static_cast<AdblockPlus::FileReader*>(external->Value());
    Slurp(*fileReader, *path);
    return v8::Undefined();
  }

  v8::Handle<v8::Context> CreateContext(
    const AdblockPlus::FileReader* const fileReader,
    AdblockPlus::ErrorCallback* const errorCallback)
  {
    const v8::HandleScope handleScope;
    const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
    const v8::Handle<v8::ObjectTemplate> libAdblockPlus =
      v8::ObjectTemplate::New();
    const v8::Handle<v8::FunctionTemplate> reportError =
      v8::FunctionTemplate::New(ReportErrorCallback,
                                v8::External::New(errorCallback));
    libAdblockPlus->Set(v8::String::New("reportError"), reportError);
    const v8::Handle<v8::FunctionTemplate> load =
      v8::FunctionTemplate::New(LoadCallback, v8::External::New(
        const_cast<AdblockPlus::FileReader*>(fileReader)));
    libAdblockPlus->Set(v8::String::New("load"), load);
    global->Set(v8::String::New("LibAdblockPlus"), libAdblockPlus);
    const v8::Persistent<v8::Context> context = v8::Context::New(0, global);
    return context;
  }
}

AdblockPlus::JsError::JsError(const v8::Handle<v8::Value> exception)
  : std::runtime_error(*v8::String::AsciiValue(exception))
{
}

AdblockPlus::JsEngine::JsEngine(const FileReader* const fileReader,
                                ErrorCallback* const errorCallback)
  : fileReader(fileReader), context(CreateContext(fileReader, errorCallback))
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
  Evaluate(Slurp(*fileReader, scriptPath));
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
