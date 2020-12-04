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

#ifndef ADBLOCK_PLUS_BASE_JS_TEST_H
#define ADBLOCK_PLUS_BASE_JS_TEST_H

#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include <thread>

#include "../src/DefaultPlatform.h"
#include "../src/JsEngine.h"
#include "../src/Thread.h"
#include "AdblockPlus/Platform.h"

// Strictly speaking in each test there should be a special implementation of
// an interface, which is merely referenced by a wrapper and the latter should
// be injected into JsEngine or what ever. However the everything a test often
// actually needs is the access to pending tasks. Therefore instantiation of
// implementation of an interface, creation of shared tasks and sharing them
// with tasks in test is located in this class.
//
// Task is passed as an additional template parameter instead of using traits
// (CRTP does not work with types in derived class) merely to simplify the code
// by minimization.
template<typename T, typename TTask, typename Interface> class DelayedMixin : public Interface
{
public:
  typedef TTask Task;
  typedef std::shared_ptr<std::list<Task>> SharedTasks;
  static std::unique_ptr<Interface> New(SharedTasks& tasks)
  {
    std::unique_ptr<T> result(new T());
    tasks = result->tasks;
    return std::move(result);
  }

protected:
  DelayedMixin() : tasks(std::make_shared<std::list<Task>>())
  {
  }

  SharedTasks tasks;
};

class ThrowingTimer : public AdblockPlus::ITimer
{
  void SetTimer(const std::chrono::milliseconds& timeout,
                const TimerCallback& timerCallback) override
  {
    throw std::runtime_error("Unexpected timer: " + std::to_string(timeout.count()));
  }
};

class NoopTimer : public AdblockPlus::ITimer
{
  void SetTimer(const std::chrono::milliseconds& timeout,
                const TimerCallback& timerCallback) override
  {
  }
};

struct DelayedTimerTask
{
  std::chrono::milliseconds timeout;
  AdblockPlus::ITimer::TimerCallback callback;
};

class DelayedTimer : public DelayedMixin<DelayedTimer, DelayedTimerTask, AdblockPlus::ITimer>
{
public:
  void SetTimer(const std::chrono::milliseconds& timeout,
                const TimerCallback& timerCallback) override
  {
    Task task = {timeout, timerCallback};
    tasks->emplace_back(task);
  }

  // JS part often schedules download requests asynchronously using promises.
  // So, we need to firstly process those timers to actually schedule web
  // requests and afterwards we may inspect pending web requests.
  // Non-immediate timers are not touched.
  static void ProcessImmediateTimers(DelayedTimer::SharedTasks& timerTasks);
};

class ThrowingLogSystem : public AdblockPlus::LogSystem
{
public:
  void operator()(LogLevel logLevel, const std::string& message, const std::string& source)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

class ThrowingFileSystem : public AdblockPlus::IFileSystem
{
public:
  void Read(const std::string& fileName,
            const ReadCallback& callback,
            const Callback& errorCallback) const override
  {
    throw std::runtime_error("Not implemented");
  }

  void Write(const std::string& fileName, const IOBuffer& data, const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Move(const std::string& fromFileName,
            const std::string& toFileName,
            const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Remove(const std::string& fileName, const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Stat(const std::string& fileName, const StatCallback& callback) const override
  {
    throw std::runtime_error("Not implemented");
  }
};

class ThrowingWebRequest : public AdblockPlus::IWebRequest
{
public:
  void GET(const std::string& url,
           const AdblockPlus::HeaderList& requestHeaders,
           const GetCallback& callback) override
  {
    throw std::runtime_error("Unexpected GET: " + url);
  }
};

class ThrowingResourceReader : public AdblockPlus::IResourceReader
{
public:
  void ReadPreloadedFilterList(const std::string& url,
                               const ReadCallback& doneCallback) const override
  {
    throw std::runtime_error("Unexpected ReadPreloadedFilterList: " + url);
  }
};

class LazyFileSystem : public AdblockPlus::IFileSystem
{
public:
  typedef std::function<void()> Task;
  typedef std::function<void(const Task& task)> Scheduler;
  static void ExecuteImmediately(const Task& task)
  {
    task();
  }
  explicit LazyFileSystem(const Scheduler& scheduler = LazyFileSystem::ExecuteImmediately)
      : scheduler(scheduler)
  {
  }

  void Read(const std::string& fileName,
            const ReadCallback& callback,
            const Callback& errorCallback) const override
  {
    scheduler([fileName, callback, errorCallback] {
      if (fileName == "patterns.ini")
      {
        std::string dummyData = "# Adblock Plus preferences\n[Subscription]\nurl=~user~0000";
        callback(IOBuffer(dummyData.cbegin(), dummyData.cend()));
      }
      else if (fileName == "prefs.json")
      {
        std::string dummyData = "{}";
        callback(IOBuffer(dummyData.cbegin(), dummyData.cend()));
      }
      else
        errorCallback("File not found, " + fileName);
    });
  }

  void Write(const std::string& fileName, const IOBuffer& data, const Callback& callback) override
  {
  }

  void Move(const std::string& fromFileName,
            const std::string& toFileName,
            const Callback& callback) override
  {
  }

  void Remove(const std::string& fileName, const Callback& callback) override
  {
  }

  void Stat(const std::string& fileName, const StatCallback& callback) const override
  {
    scheduler([fileName, callback] {
      StatResult result;
      if (fileName == "patterns.ini")
      {
        result.exists = true;
      }
      callback(result, "");
    });
  }

protected:
  Scheduler scheduler;
};

class InMemoryFileSystem : public LazyFileSystem
{
  std::map<std::string, IOBuffer> files;

public:
  using LazyFileSystem::LazyFileSystem;
  void Read(const std::string& fileName,
            const ReadCallback& callback,
            const Callback& errorCallback) const override
  {
    scheduler([this, fileName, callback, errorCallback]() {
      auto ii_file = files.find(fileName);
      if (ii_file != files.end())
        callback(IOBuffer(ii_file->second));
      else
        errorCallback("File not found, " + fileName);
    });
  }

  void Write(const std::string& fileName, const IOBuffer& data, const Callback& callback) override
  {
    scheduler([this, fileName, data, callback]() {
      files[fileName] = data;
      callback("");
    });
  }

  void Move(const std::string& fromFileName,
            const std::string& toFileName,
            const Callback& callback) override
  {
    scheduler([this, fromFileName, toFileName, callback]() {
      auto ii_fromFile = files.find(fromFileName);
      if (ii_fromFile == files.end())
      {
        callback("File (from) not found, " + fromFileName);
        return;
      }
      Write(toFileName,
            ii_fromFile->second,
            [this, fromFileName, callback](const std::string& error) {
              if (!error.empty())
              {
                callback(error);
                return;
              }
              Remove(fromFileName, callback);
            });
    });
  }

  void Remove(const std::string& fileName, const Callback& callback) override
  {
    scheduler([this, fileName, callback]() {
      files.erase(fileName);
      callback("");
    });
  }

  void Stat(const std::string& fileName, const StatCallback& callback) const override
  {
    scheduler([this, fileName, callback]() {
      StatResult result;
      result.exists = files.find(fileName) != files.end();
      callback(result, "");
    });
  }
};

AdblockPlus::IFilterEngine&
CreateFilterEngine(AdblockPlus::Platform& platform,
                   const AdblockPlus::FilterEngineFactory::CreationParameters& creationParams =
                       AdblockPlus::FilterEngineFactory::CreationParameters());

class NoopWebRequest : public AdblockPlus::IWebRequest
{
public:
  void GET(const std::string& url,
           const AdblockPlus::HeaderList& requestHeaders,
           const GetCallback& callback) override
  {
  }
};

class WrappingWebRequest : public AdblockPlus::IWebRequest
{
public:
  typedef std::function<void(
      const std::string&, const AdblockPlus::HeaderList&, const GetCallback&)>
      Implementation;

  WrappingWebRequest(Implementation callback) : impl(callback)
  {
  }

  void GET(const std::string& url,
           const AdblockPlus::HeaderList& requestHeaders,
           const GetCallback& callback) override
  {
    impl(url, requestHeaders, callback);
  }

  Implementation impl;
};

struct DelayedWebRequestTask
{
  std::string url;
  AdblockPlus::HeaderList headers;
  AdblockPlus::IWebRequest::GetCallback getCallback;
};

class DelayedWebRequest
    : public DelayedMixin<DelayedWebRequest, DelayedWebRequestTask, AdblockPlus::IWebRequest>
{
public:
  void GET(const std::string& url,
           const AdblockPlus::HeaderList& requestHeaders,
           const GetCallback& callback) override
  {
    Task task = {url, requestHeaders, callback};
    tasks->emplace_back(task);
  }
};

class LazyLogSystem : public AdblockPlus::LogSystem
{
public:
  void operator()(LogLevel logLevel, const std::string& message, const std::string& source)
  {
  }
};

struct ThrowingPlatformCreationParameters : AdblockPlus::PlatformFactory::CreationParameters
{
  ThrowingPlatformCreationParameters();
};

class BaseJsTest : public ::testing::Test
{
protected:
  std::unique_ptr<AdblockPlus::Platform> platform;

  BaseJsTest()
  {
    platform = AdblockPlus::PlatformFactory::CreatePlatform(ThrowingPlatformCreationParameters());
  }

  AdblockPlus::JsEngine& GetJsEngine()
  {
    if (!platform)
      throw std::runtime_error("Platform must be initialized");
    return static_cast<AdblockPlus::DefaultPlatform*>(platform.get())->GetJsEngine();
  }
};

#endif // ADBLOCK_PLUS_BASE_JS_TEST_H
