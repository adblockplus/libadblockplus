#include <AdblockPlus/FileSystem.h>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>
#include "FileSystemJsObject.h"
#include "Utils.h"
#include "Thread.h"
#include "Utils.h"

using namespace AdblockPlus;

namespace
{
  class IoThread : public Thread
  {
  public:
    IoThread(JsEngine& jsEngine, JsValuePtr callback)
      : jsEngine(jsEngine), fileSystem(jsEngine.GetFileSystem()),
        callback(callback)
    {
    }

  protected:
    JsEngine& jsEngine;
    FileSystemPtr fileSystem;
    JsValuePtr callback;
  };

  class ReadThread : public IoThread
  {
  public:
    ReadThread(JsEngine& jsEngine, JsValuePtr callback,
               const std::string& path)
      : IoThread(jsEngine, callback), path(path)
    {
    }

    void Run()
    {
      std::string content;
      std::string error;
      try
      {
        std::tr1::shared_ptr<std::istream> stream = fileSystem->Read(path);
        content = Utils::Slurp(*stream);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error =  "Unknown error while reading from " + path;
      }

      const JsEngine::Context context(jsEngine);
      JsValuePtr result = jsEngine.NewObject();
      result->SetProperty("content", content);
      result->SetProperty("error", error);
      JsValueList params;
      params.push_back(result);
      callback->Call(params);
      delete this;
    }

  private:
    std::string path;
  };

  class WriteThread : public IoThread
  {
  public:
    WriteThread(JsEngine& jsEngine, JsValuePtr callback,
                const std::string& path, const std::string& content)
      : IoThread(jsEngine, callback), path(path), content(content)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        std::tr1::shared_ptr<std::ostream> stream(new std::stringstream);
        *stream << content;
        fileSystem->Write(path, stream);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while writing to " + path;
      }

      const JsEngine::Context context(jsEngine);
      JsValuePtr errorValue = jsEngine.NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback->Call(params);
      delete this;
    }

  private:
    std::string path;
    std::string content;
  };

  class MoveThread : public IoThread
  {
  public:
    MoveThread(JsEngine& jsEngine, JsValuePtr callback,
               const std::string& fromPath, const std::string& toPath)
      : IoThread(jsEngine, callback), fromPath(fromPath), toPath(toPath)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem->Move(fromPath, toPath);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while moving " + fromPath + " to " + toPath;
      }

      const JsEngine::Context context(jsEngine);
      JsValuePtr errorValue = jsEngine.NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback->Call(params);
      delete this;
    }

  private:
    std::string fromPath;
    std::string toPath;
  };

  class RemoveThread : public IoThread
  {
  public:
    RemoveThread(JsEngine& jsEngine, JsValuePtr callback,
                 const std::string& path)
      : IoThread(jsEngine, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem->Remove(path);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while removing " + path;
      }

      const JsEngine::Context context(jsEngine);
      JsValuePtr errorValue = jsEngine.NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback->Call(params);
      delete this;
    }

  private:
    std::string path;
  };

  class StatThread : public IoThread
  {
  public:
    StatThread(JsEngine& jsEngine, JsValuePtr callback,
               const std::string& path)
      : IoThread(jsEngine, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      FileSystem::StatResult statResult;
      try
      {
        statResult = fileSystem->Stat(path);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while calling stat on " + path;
      }

      const JsEngine::Context context(jsEngine);
      JsValuePtr result = jsEngine.NewObject();
      result->SetProperty("exists", statResult.exists);
      result->SetProperty("isFile", statResult.isFile);
      result->SetProperty("isDirectory", statResult.isDirectory);
      result->SetProperty("lastModified", statResult.lastModified);
      result->SetProperty("error", error);

      JsValueList params;
      params.push_back(result);
      callback->Call(params);
      delete this;
    }

  private:
    std::string path;
  };

  v8::Handle<v8::Value> ReadCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    if (converted.size() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.read requires 2 parameters"));
    if (!converted[1]->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.read must be a function"));
    ReadThread* const readThread = new ReadThread(jsEngine, converted[1],
        converted[0]->AsString());
    readThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> WriteCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    if (converted.size() != 3)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.write requires 3 parameters"));
    if (!converted[2]->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Third argument to _fileSystem.write must be a function"));
    WriteThread* const writeThread = new WriteThread(jsEngine, converted[2],
        converted[0]->AsString(), converted[1]->AsString());
    writeThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> MoveCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    if (converted.size() != 3)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.move requires 3 parameters"));
    if (!converted[2]->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Third argument to _fileSystem.move must be a function"));
    MoveThread* const moveThread = new MoveThread(jsEngine, converted[2],
        converted[0]->AsString(), converted[1]->AsString());
    moveThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> RemoveCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    if (converted.size() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.remove requires 2 parameters"));
    if (!converted[1]->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.remove must be a function"));
    RemoveThread* const removeThread = new RemoveThread(jsEngine, converted[1],
        converted[0]->AsString());
    removeThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> StatCallback(const v8::Arguments& arguments)
  {
    AdblockPlus::JsEngine& jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine.ConvertArguments(arguments);

    if (converted.size() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.stat requires 2 parameters"));
    if (!converted[1]->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.stat must be a function"));
    StatThread* const statThread = new StatThread(jsEngine, converted[1],
        converted[0]->AsString());
    statThread->Start();
    return v8::Undefined();
  }
}

JsValuePtr FileSystemJsObject::Setup(JsEngine& jsEngine, JsValuePtr obj)
{
  obj->SetProperty("read", jsEngine.NewCallback(::ReadCallback));
  obj->SetProperty("write", jsEngine.NewCallback(::WriteCallback));
  obj->SetProperty("move", jsEngine.NewCallback(::MoveCallback));
  obj->SetProperty("remove", jsEngine.NewCallback(::RemoveCallback));
  obj->SetProperty("stat", jsEngine.NewCallback(::StatCallback));
  return obj;
}
