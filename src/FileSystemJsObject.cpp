/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AdblockPlus/FileSystem.h>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <AdblockPlus/JsValue.h>
#include "FileSystemJsObject.h"
#include "JsContext.h"
#include "Thread.h"
#include "Utils.h"

using namespace AdblockPlus;
using AdblockPlus::Utils::ThrowExceptionInJS;

namespace
{
  class IoThread : public Thread
  {
  public:
    IoThread(const JsEnginePtr& jsEngine, const JsValue& callback)
      : Thread(true), jsEngine(jsEngine), fileSystem(jsEngine->GetFileSystem()),
        callback(callback)
    {
    }

  protected:
    JsEnginePtr jsEngine;
    FileSystemPtr fileSystem;
    JsValue callback;
  };

  class ReadThread : public IoThread
  {
  public:
    ReadThread(const JsEnginePtr& jsEngine, const JsValue& callback,
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
        std::shared_ptr<std::istream> stream = fileSystem->Read(path);
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

      const JsContext context(*jsEngine);
      auto result = jsEngine->NewObject();
      result.SetProperty("content", content);
      result.SetProperty("error", error);
      JsValueList params;
      params.push_back(result);
      callback.Call(params);
    }

  private:
    std::string path;
  };

  class WriteThread : public IoThread
  {
  public:
    WriteThread(const JsEnginePtr& jsEngine, const JsValue& callback,
                const std::string& path, const std::string& content)
      : IoThread(jsEngine, callback), path(path), content(content)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        std::stringstream stream;
        stream << content;
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

      const JsContext context(*jsEngine);
      auto errorValue = jsEngine->NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback.Call(params);
    }

  private:
    std::string path;
    std::string content;
  };

  class MoveThread : public IoThread
  {
  public:
    MoveThread(const JsEnginePtr& jsEngine, const JsValue& callback,
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

      const JsContext context(*jsEngine);
      auto errorValue = jsEngine->NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback.Call(params);
    }

  private:
    std::string fromPath;
    std::string toPath;
  };

  class RemoveThread : public IoThread
  {
  public:
    RemoveThread(const JsEnginePtr& jsEngine, const JsValue& callback,
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

      const JsContext context(*jsEngine);
      auto errorValue = jsEngine->NewValue(error);
      JsValueList params;
      params.push_back(errorValue);
      callback.Call(params);
    }

  private:
    std::string path;
  };


  class StatThread : public IoThread
  {
  public:
    StatThread(const JsEnginePtr& jsEngine, const JsValue& callback,
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

      const JsContext context(*jsEngine);
      auto result = jsEngine->NewObject();
      result.SetProperty("exists", statResult.exists);
      result.SetProperty("isFile", statResult.isFile);
      result.SetProperty("isDirectory", statResult.isDirectory);
      result.SetProperty("lastModified", statResult.lastModified);
      result.SetProperty("error", error);

      JsValueList params;
      params.push_back(result);
      callback.Call(params);
    }

  private:
    std::string path;
  };

  void ReadCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 2)
      return ThrowExceptionInJS(isolate, "_fileSystem.read requires 2 parameters");
    if (!converted[1].IsFunction())
      return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.read must be a function");
    ReadThread* const readThread = new ReadThread(jsEngine, converted[1],
        converted[0].AsString());
    readThread->Start();
  }

  void WriteCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 3)
      return ThrowExceptionInJS(isolate, "_fileSystem.write requires 3 parameters");
    if (!converted[2].IsFunction())
      return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.write must be a function");
    WriteThread* const writeThread = new WriteThread(jsEngine, converted[2],
        converted[0].AsString(), converted[1].AsString());
    writeThread->Start();
  }

  void MoveCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 3)
      return ThrowExceptionInJS(isolate, "_fileSystem.move requires 3 parameters");
    if (!converted[2].IsFunction())
      return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.move must be a function");
    MoveThread* const moveThread = new MoveThread(jsEngine, converted[2],
        converted[0].AsString(), converted[1].AsString());
    moveThread->Start();
  }

  void RemoveCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 2)
      return ThrowExceptionInJS(isolate, "_fileSystem.remove requires 2 parameters");
    if (!converted[1].IsFunction())
      return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.remove must be a function");
    RemoveThread* const removeThread = new RemoveThread(jsEngine, converted[1],
        converted[0].AsString());
    removeThread->Start();
  }

  void StatCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 2)
      return ThrowExceptionInJS(isolate, "_fileSystem.stat requires 2 parameters");
    if (!converted[1].IsFunction())
      return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.stat must be a function");
    StatThread* const statThread = new StatThread(jsEngine, converted[1],
        converted[0].AsString());
    statThread->Start();
  }

  void ResolveCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEnginePtr jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 1)
      return ThrowExceptionInJS(isolate, "_fileSystem.resolve requires 1 parameter");

    std::string resolved = jsEngine->GetFileSystem()->Resolve(converted[0].AsString());
    arguments.GetReturnValue().Set(Utils::ToV8String(isolate, resolved));
  }
}


JsValue& FileSystemJsObject::Setup(JsEngine& jsEngine, JsValue& obj)
{
  obj.SetProperty("read", jsEngine.NewCallback(::ReadCallback));
  obj.SetProperty("write", jsEngine.NewCallback(::WriteCallback));
  obj.SetProperty("move", jsEngine.NewCallback(::MoveCallback));
  obj.SetProperty("remove", jsEngine.NewCallback(::RemoveCallback));
  obj.SetProperty("stat", jsEngine.NewCallback(::StatCallback));
  obj.SetProperty("resolve", jsEngine.NewCallback(::ResolveCallback));
  return obj;
}
