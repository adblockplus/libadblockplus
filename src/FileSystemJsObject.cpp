#include <AdblockPlus/FileSystem.h>
#include <stdexcept>
#include <sstream>
#include <vector>

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
    IoThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback)
      : isolate(v8::Isolate::GetCurrent()),
        context(v8::Persistent<v8::Context>::New(isolate,
                                                 v8::Context::GetCurrent())),
        fileSystem(fileSystem),
        callback(v8::Persistent<v8::Function>::New(isolate, callback))
    {
    }

    virtual ~IoThread()
    {
      callback.Dispose(isolate);
      context.Dispose(isolate);
    }

  protected:
    v8::Isolate* const isolate;
    v8::Persistent<v8::Context> context;
    FileSystem& fileSystem;
    v8::Persistent<v8::Function> callback;
  };

  class ReadThread : public IoThread
  {
  public:
    ReadThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback,
               const std::string& path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string content;
      std::string error;
      try
      {
        std::tr1::shared_ptr<std::istream> stream = fileSystem.Read(path);
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

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle<v8::Object> result = v8::Object::New();
      result->Set(v8::String::New("content"), Utils::ToV8String(content));
      result->Set(v8::String::New("error"), Utils::ToV8String(error));
      callback->Call(callback, 1,
                     reinterpret_cast<v8::Handle<v8::Value>*>(&result));
      delete this;
    }

  private:
    std::string path;
  };

  class WriteThread : public IoThread
  {
  public:
    WriteThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback,
                const std::string& path, const std::string& content)
      : IoThread(fileSystem, callback), path(path), content(content)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        std::tr1::shared_ptr<std::ostream> stream(new std::stringstream);
        *stream << content;
        fileSystem.Write(path, stream);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while writing to " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle<v8::Value> errorValue = Utils::ToV8String(error);
      callback->Call(callback, 1, &errorValue);
      delete this;
    }

  private:
    std::string path;
    std::string content;
  };

  class MoveThread : public IoThread
  {
  public:
    MoveThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback,
               const std::string& fromPath, const std::string& toPath)
      : IoThread(fileSystem, callback), fromPath(fromPath), toPath(toPath)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem.Move(fromPath, toPath);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while moving " + fromPath + " to " + toPath;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle<v8::Value> errorValue = Utils::ToV8String(error);
      callback->Call(callback, 1, &errorValue);
      delete this;
    }

  private:
    std::string fromPath;
    std::string toPath;
  };

  class RemoveThread : public IoThread
  {
  public:
    RemoveThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback,
                 const std::string& path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem.Remove(path);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while removing " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle<v8::Value> errorValue = Utils::ToV8String(error);
      callback->Call(callback, 1, &errorValue);
      delete this;
    }

  private:
    std::string path;
  };

  class StatThread : public IoThread
  {
  public:
    StatThread(FileSystem& fileSystem, v8::Handle<v8::Function> callback,
               const std::string& path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      FileSystem::StatResult statResult;
      try
      {
        statResult = fileSystem.Stat(path);
      }
      catch (std::exception& e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while calling stat on " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle<v8::Object> result = v8::Object::New();
      result->Set(v8::String::New("exists"),
                  v8::Boolean::New(statResult.exists));
      result->Set(v8::String::New("isFile"),
                  v8::Boolean::New(statResult.isFile));
      result->Set(v8::String::New("isDirectory"),
                  v8::Boolean::New(statResult.isDirectory));
      result->Set(v8::String::New("lastModified"),
                  v8::Integer::New(statResult.lastModified));
      result->Set(v8::String::New("error"), Utils::ToV8String(error));
      callback->Call(callback, 1,
                     reinterpret_cast<v8::Handle<v8::Value>*>(&result));
      delete this;
    }

  private:
    std::string path;
  };

  v8::Handle<v8::Value> ReadCallback(const v8::Arguments& arguments)
  {
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast<FileSystem*>(external->Value());
    if (arguments.Length() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.read requires 2 parameters"));
    const std::string path = Utils::FromV8String(arguments[0]);
    v8::Handle<v8::Value> callbackValue = arguments[1];
    if (!callbackValue->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.read must be a function"));
    v8::Handle<v8::Function> callback =
      v8::Handle<v8::Function>::Cast(callbackValue);
    ReadThread* const readThread = new ReadThread(*fileSystem, callback, path);
    readThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> WriteCallback(const v8::Arguments& arguments)
  {
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast<FileSystem*>(external->Value());
    if (arguments.Length() != 3)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.write requires 3 parameters"));
    const std::string path = Utils::FromV8String(arguments[0]);
    const std::string content = Utils::FromV8String(arguments[1]);
    v8::Handle<v8::Value> callbackValue = arguments[2];
    if (!callbackValue->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Third argument to _fileSystem.write must be a function"));
    v8::Handle<v8::Function> callback =
      v8::Handle<v8::Function>::Cast(callbackValue);
    WriteThread* const writeThread = new WriteThread(*fileSystem, callback,
                                                     path, content);
    writeThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> MoveCallback(const v8::Arguments& arguments)
  {
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast<FileSystem*>(external->Value());
    if (arguments.Length() != 3)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.move requires 3 parameters"));
    const std::string fromPath = Utils::FromV8String(arguments[0]);
    const std::string toPath = Utils::FromV8String(arguments[1]);
    v8::Handle<v8::Value> callbackValue = arguments[2];
    if (!callbackValue->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Third argument to _fileSystem.move must be a function"));
    v8::Handle<v8::Function> callback =
      v8::Handle<v8::Function>::Cast(callbackValue);
    MoveThread* const moveThread = new MoveThread(*fileSystem, callback,
                                                  fromPath, toPath);
    moveThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> RemoveCallback(const v8::Arguments& arguments)
  {
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast<FileSystem*>(external->Value());
    if (arguments.Length() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.remove requires 2 parameters"));
    const std::string path = Utils::FromV8String(arguments[0]);
    v8::Handle<v8::Value> callbackValue = arguments[1];
    if (!callbackValue->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.remove must be a function"));
    v8::Handle<v8::Function> callback =
      v8::Handle<v8::Function>::Cast(callbackValue);
    RemoveThread* const removeThread = new RemoveThread(*fileSystem, callback,
                                                        path);
    removeThread->Start();
    return v8::Undefined();
  }

  v8::Handle<v8::Value> StatCallback(const v8::Arguments& arguments)
  {
    const v8::Handle<const v8::External> external =
      v8::Handle<const v8::External>::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast<FileSystem*>(external->Value());
    if (arguments.Length() != 2)
      return v8::ThrowException(v8::String::New(
        "_fileSystem.stat requires 2 parameters"));
    const std::string path = Utils::FromV8String(arguments[0]);
    v8::Handle<v8::Value> callbackValue = arguments[1];
    if (!callbackValue->IsFunction())
      return v8::ThrowException(v8::String::New(
        "Second argument to _fileSystem.stat must be a function"));
    v8::Handle<v8::Function> callback =
      v8::Handle<v8::Function>::Cast(callbackValue);
    StatThread* const statThread = new StatThread(*fileSystem, callback, path);
    statThread->Start();
    return v8::Undefined();
  }
}

v8::Handle<v8::ObjectTemplate>
FileSystemJsObject::Create(FileSystem& fileSystem)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> file = v8::ObjectTemplate::New();
  const v8::Local<v8::External> fileSystemExternal =
    v8::External::New(&fileSystem);
  file->Set(v8::String::New("read"),
            v8::FunctionTemplate::New(ReadCallback, fileSystemExternal));
  file->Set(v8::String::New("write"),
            v8::FunctionTemplate::New(WriteCallback, fileSystemExternal));
  file->Set(v8::String::New("move"),
            v8::FunctionTemplate::New(MoveCallback, fileSystemExternal));
  file->Set(v8::String::New("remove"),
            v8::FunctionTemplate::New(RemoveCallback, fileSystemExternal));
  file->Set(v8::String::New("stat"),
            v8::FunctionTemplate::New(StatCallback, fileSystemExternal));
  return handleScope.Close(file);
}
