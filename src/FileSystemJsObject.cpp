/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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

#include <AdblockPlus/IFileSystem.h>
#include <stdexcept>
#include <sstream>
#include <vector>

#include <AdblockPlus/JsValue.h>
#include "FileSystemJsObject.h"
#include "JsContext.h"
#include "Utils.h"
#include "JsError.h"
#include <AdblockPlus/Platform.h>

using namespace AdblockPlus;
using AdblockPlus::Utils::ThrowExceptionInJS;

namespace
{
  namespace ReadCallback
  {
    struct WeakData
    {
    public:
      WeakData(JsEngine& jsEngine,
        JsEngine::JsWeakValuesID weakResolveCallback,
        JsEngine::JsWeakValuesID weakRejectCallback)
        : jsEngine(jsEngine)
        , weakResolveCallback(weakResolveCallback)
        , weakRejectCallback(weakRejectCallback)
      {
      }
      virtual ~WeakData()
      {
        jsEngine.TakeJsValues(weakResolveCallback);
        jsEngine.TakeJsValues(weakRejectCallback);
      }
      JsEngine& jsEngine;
      JsEngine::JsWeakValuesID weakResolveCallback;
      JsEngine::JsWeakValuesID weakRejectCallback;
    };

    static void V8Callback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
    {
      AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
      AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

      v8::Isolate* isolate = arguments.GetIsolate();
      if (converted.size() != 3)
        return ThrowExceptionInJS(isolate, "_fileSystem.read requires 3 parameters");
      if (!converted[1].IsFunction())
        return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.read must be a function");
      if (!converted[2].IsFunction())
        return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.read must be a function");

      auto weakResolveCallback = jsEngine->StoreJsValues({converted[1]});
      auto weakRejectCallback = jsEngine->StoreJsValues({converted[2]});
      auto weakData = std::make_shared<WeakData>(*jsEngine, weakResolveCallback,
          weakRejectCallback);
      auto fileName = converted[0].AsString();
      jsEngine->GetPlatform().WithFileSystem(
        [jsEngine, weakData, fileName](IFileSystem& fileSystem)
        {
          fileSystem.Read(fileName, [jsEngine, weakData](IFileSystem::IOBuffer&& content)
            {
              const JsContext context(*jsEngine);
              auto result = jsEngine->NewObject();
              result.SetStringBufferProperty("content", content);
              jsEngine->GetJsValues(weakData->weakResolveCallback)[0].Call(result);
            },
            [jsEngine, weakData](const std::string& error)
            {
              if (error.empty())
                return;
              const JsContext context(*jsEngine);
              jsEngine->GetJsValues(weakData->weakRejectCallback)[0].Call(jsEngine->NewValue(error));
            });
        });
    } // V8Callback
  } // namespace ReadCallback

  namespace ReadFromFileCallback
  {
    inline bool IsEndOfLine(char c)
    {
      return c == 10 || c == 13;
    }

    inline StringBuffer::const_iterator SkipEndOfLine(StringBuffer::const_iterator ii, StringBuffer::const_iterator end)
    {
      while (ii != end && IsEndOfLine(*ii))
        ++ii;
      return ii;
    }

    inline StringBuffer::const_iterator AdvanceToEndOfLine(StringBuffer::const_iterator ii, StringBuffer::const_iterator end)
    {
      while (ii != end && !IsEndOfLine(*ii))
        ++ii;
      return ii;
    }

    struct WeakData : ReadCallback::WeakData
    {
    public:
      WeakData(JsEngine& jsEngine,
        JsEngine::JsWeakValuesID weakResolveCallback,
        JsEngine::JsWeakValuesID weakRejectCallback,
        JsEngine::JsWeakValuesID weakProcessFunc)
        : ReadCallback::WeakData(jsEngine, weakResolveCallback, weakRejectCallback)
        , weakProcessFunc(weakProcessFunc)
      {
      }
      ~WeakData()
      {
        jsEngine.TakeJsValues(weakProcessFunc);
      }
      JsEngine::JsWeakValuesID weakProcessFunc;
    };

    void V8Callback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
    {
      AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
      AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

      v8::Isolate* isolate = arguments.GetIsolate();
      if (converted.size() != 4)
        return ThrowExceptionInJS(isolate, "_fileSystem.readFromFile requires 4 parameters");
      if (!converted[1].IsFunction())
        return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.readFromFile must be a function (listener callback)");
      if (!converted[2].IsFunction())
        return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.readFromFile must be a function (done callback)");
      if (!converted[3].IsFunction())
        return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.readFromFile must be a function (error callback)");

      auto weakProcessFunc = jsEngine->StoreJsValues({converted[1]});
      auto weakResolveCallback = jsEngine->StoreJsValues({converted[2]});
      auto weakRejectCallback = jsEngine->StoreJsValues({converted[3]});
      auto weakData = std::make_shared<WeakData>(
          *jsEngine, weakResolveCallback, weakRejectCallback, weakProcessFunc);
      auto fileName = converted[0].AsString();
      jsEngine->GetPlatform().WithFileSystem([jsEngine, weakData, fileName](IFileSystem& fileSystem)
        {
          fileSystem.Read(fileName, [jsEngine, weakData](IFileSystem::IOBuffer&& content)
            {
              const JsContext context(*jsEngine);
              auto jsValues = jsEngine->GetJsValues(weakData->weakProcessFunc);
              auto processFunc = jsValues[0].UnwrapValue().As<v8::Function>();
              auto globalContext = context.GetV8Context()->Global();
              if (!globalContext->IsObject())
                throw std::runtime_error("`this` pointer has to be an object");

              auto isolate = jsEngine->GetIsolate();
              const v8::TryCatch tryCatch(isolate);
              const auto contentEnd = content.cend();
              auto stringBegin = SkipEndOfLine(content.begin(), contentEnd);
              auto v8Context = isolate->GetCurrentContext();
              do
              {
                auto stringEnd = AdvanceToEndOfLine(stringBegin, contentEnd);
                auto jsLine = CHECKED_TO_LOCAL_WITH_TRY_CATCH(
                  isolate,
                  Utils::StringBufferToV8String(
                    isolate, StringBuffer(stringBegin, stringEnd)),
                  tryCatch).As<v8::Value>();

                CHECKED_TO_LOCAL_WITH_TRY_CATCH(
                  isolate, processFunc->Call(v8Context, globalContext, 1, &jsLine),
                  tryCatch);

                stringBegin = SkipEndOfLine(stringEnd, contentEnd);
              } while (stringBegin != contentEnd);
              jsEngine->GetJsValues(weakData->weakResolveCallback)[0].Call();
            }, [jsEngine, weakData](const std::string& error)
            {
              if (error.empty())
                return;
              jsEngine->GetJsValues(weakData->weakRejectCallback)[0].Call(jsEngine->NewValue(error));
            });
        });
    } // V8Callback
  } // namespace ReadFromFileCallback

  void WriteCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 3)
      return ThrowExceptionInJS(isolate, "_fileSystem.write requires 3 parameters");
    if (!converted[2].IsFunction())
      return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.write must be a function");

    JsValueList values;
    values.push_back(converted[2]);
    auto weakCallback = jsEngine->StoreJsValues(values);
    auto content = converted[1].AsStringBuffer();
    auto fileName = converted[0].AsString();
    jsEngine->GetPlatform().WithFileSystem(
      [jsEngine, weakCallback, fileName, content](IFileSystem& fileSystem)
      {
        fileSystem.Write(fileName, content,
          [jsEngine, weakCallback](const std::string& error)
          {
            const JsContext context(*jsEngine);
            JsValueList params;
            if (!error.empty())
              params.push_back(jsEngine->NewValue(error));
            jsEngine->TakeJsValues(weakCallback)[0].Call(params);
          });
      });
  }

  void MoveCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 3)
      return ThrowExceptionInJS(isolate, "_fileSystem.move requires 3 parameters");
    if (!converted[2].IsFunction())
      return ThrowExceptionInJS(isolate, "Third argument to _fileSystem.move must be a function");

    JsValueList values;
    values.push_back(converted[2]);
    auto weakCallback = jsEngine->StoreJsValues(values);
    auto from = converted[0].AsString();
    auto to = converted[1].AsString();
    jsEngine->GetPlatform().WithFileSystem(
      [jsEngine, weakCallback, from, to](IFileSystem& fileSystem)
      {
        fileSystem.Move(from, to,
          [jsEngine, weakCallback](const std::string& error)
          {
            const JsContext context(*jsEngine);
            JsValueList params;
            if (!error.empty())
              params.push_back(jsEngine->NewValue(error));
            jsEngine->TakeJsValues(weakCallback)[0].Call(params);
          });
      });
  }

  void RemoveCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 2)
      return ThrowExceptionInJS(isolate, "_fileSystem.remove requires 2 parameters");
    if (!converted[1].IsFunction())
      return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.remove must be a function");

    JsValueList values;
    values.push_back(converted[1]);
    auto weakCallback = jsEngine->StoreJsValues(values);
    auto fileName = converted[0].AsString();
    jsEngine->GetPlatform().WithFileSystem(
      [jsEngine, weakCallback, fileName](IFileSystem& fileSystem)
      {
        fileSystem.Remove(fileName,
          [jsEngine, weakCallback](const std::string& error)
          {
            const JsContext context(*jsEngine);
            JsValueList params;
            if (!error.empty())
              params.push_back(jsEngine->NewValue(error));
            jsEngine->TakeJsValues(weakCallback)[0].Call(params);
          });
      });
  }

  void StatCallback(const v8::FunctionCallbackInfo<v8::Value>& arguments)
  {
    AdblockPlus::JsEngine* jsEngine = AdblockPlus::JsEngine::FromArguments(arguments);
    AdblockPlus::JsValueList converted = jsEngine->ConvertArguments(arguments);

    v8::Isolate* isolate = arguments.GetIsolate();
    if (converted.size() != 2)
      return ThrowExceptionInJS(isolate, "_fileSystem.stat requires 2 parameters");
    if (!converted[1].IsFunction())
      return ThrowExceptionInJS(isolate, "Second argument to _fileSystem.stat must be a function");

    JsValueList values;
    values.push_back(converted[1]);
    auto weakCallback = jsEngine->StoreJsValues(values);
    auto fileName = converted[0].AsString();
    jsEngine->GetPlatform().WithFileSystem(
      [jsEngine, weakCallback, fileName](IFileSystem& fileSystem)
      {
        fileSystem.Stat(fileName,
           [jsEngine, weakCallback]
           (const IFileSystem::StatResult& statResult, const std::string& error)
           {
             const JsContext context(*jsEngine);
             auto result = jsEngine->NewObject();

             result.SetProperty("exists", statResult.exists);
             result.SetProperty("lastModified", statResult.lastModified);
             if (!error.empty())
               result.SetProperty("error", error);

             JsValueList params;
             params.push_back(result);
             jsEngine->TakeJsValues(weakCallback)[0].Call(params);
           });
      });
  }
}


JsValue& FileSystemJsObject::Setup(JsEngine& jsEngine, JsValue& obj)
{
  obj.SetProperty("read", jsEngine.NewCallback(::ReadCallback::V8Callback));
  obj.SetProperty("readFromFile", jsEngine.NewCallback(::ReadFromFileCallback::V8Callback));
  obj.SetProperty("write", jsEngine.NewCallback(::WriteCallback));
  obj.SetProperty("move", jsEngine.NewCallback(::MoveCallback));
  obj.SetProperty("remove", jsEngine.NewCallback(::RemoveCallback));
  obj.SetProperty("stat", jsEngine.NewCallback(::StatCallback));
  return obj;
}
