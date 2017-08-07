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

#ifndef MOCKS_H
#define MOCKS_H

#include <thread>

#include <AdblockPlus.h>
#include <AdblockPlus/Platform.h>
#include <gtest/gtest.h>
#include "../src/Thread.h"

// Strictly speaking in each test there should be a special implementation of
// an interface, which is merely referenced by a wrapper and the latter should
// be injected into JsEngine or what ever. However the everthing a test often
// actually needs is the access to pending tasks. Therefore instantiation of
// implemenation of an interface, creation of shared tasks and sharing them
// with tasks in test is located in this class.
//
// Task is passed as an additional template parameter instead of using traits
// (CRTP does not work with types in derived class) merely to simplify the code
// by minimization.
template<typename T, typename TTask, typename Interface>
class DelayedMixin : public Interface
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
  DelayedMixin()
    : tasks(std::make_shared<std::list<Task>>())
  {
  }

  SharedTasks tasks;
};

class ThrowingTimer : public AdblockPlus::ITimer
{
  void SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback) override
  {
    throw std::runtime_error("Unexpected timer: " + std::to_string(timeout.count()));
  }
};

class NoopTimer : public AdblockPlus::ITimer
{
  void SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback) override
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
  void SetTimer(const std::chrono::milliseconds& timeout, const TimerCallback& timerCallback) override
  {
    Task task = { timeout, timerCallback };
    tasks->emplace_back(task);
  }

  // JS part often schedules download requests using Utils.runAsync which calls
  // setTimeout(callback, 0). So, we need to firstly process those timers
  // to actually schedule web requests and afterwards we may inspect pending
  // web requests.
  // non-immeditate timers are not touched
  static void ProcessImmediateTimers(DelayedTimer::SharedTasks& timerTasks);
};


class ThrowingLogSystem : public AdblockPlus::LogSystem
{
public:
  void operator()(LogLevel logLevel, const std::string& message,
        const std::string& source)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

class ThrowingFileSystem : public AdblockPlus::IFileSystem
{
public:
  void Read(const std::string& path, const ReadCallback& callback) const override
  {
    throw std::runtime_error("Not implemented");
  }

  void Write(const std::string& path, const IOBuffer& data,
    const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Move(const std::string& fromPath, const std::string& toPath,
            const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Remove(const std::string& path, const Callback& callback) override
  {
    throw std::runtime_error("Not implemented");
  }

  void Stat(const std::string& path, const StatCallback& callback) const override
  {
    throw std::runtime_error("Not implemented");
  }

  std::string Resolve(const std::string& path) const override
  {
    throw std::runtime_error("Not implemented");
  }
};

class ThrowingWebRequest : public AdblockPlus::IWebRequest
{
public:
  void GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders, const GetCallback&) override
  {
    throw std::runtime_error("Unexpected GET: " + url);
  }
};

class LazyFileSystem : public AdblockPlus::IFileSystem
{
public:
  typedef std::function<void()> Task;
  typedef std::function<void(const Task& task)> Scheduler;
  static void ExecuteImmediately(const Task& task)
  {
    if (task)
      task();
  }
  explicit LazyFileSystem(const Scheduler& scheduler = LazyFileSystem::ExecuteImmediately)
    : scheduler(scheduler)
  {
  }

  void Read(const std::string& path, const ReadCallback& callback) const override
  {
    scheduler([path, callback]
    {
      if (path == "patterns.ini")
      {
        std::string dummyData = "# Adblock Plus preferences\n[Subscription]\nurl=~fl~";
        callback(IOBuffer(dummyData.cbegin(), dummyData.cend()), "");
      }
      else if (path == "prefs.json")
      {
        std::string dummyData = "{}";
        callback(IOBuffer(dummyData.cbegin(), dummyData.cend()), "");
      }
    });
  }

  void Write(const std::string& path, const IOBuffer& data,
             const Callback& callback) override
  {
  }


  void Move(const std::string& fromPath, const std::string& toPath,
            const Callback& callback) override
  {
  }

  void Remove(const std::string& path, const Callback& callback) override
  {
  }

  void Stat(const std::string& path, const StatCallback& callback) const override
  {
    scheduler([path, callback]
    {
      StatResult result;
      if (path == "patterns.ini")
      {
        result.exists = true;
        result.isFile = true;
      }
      callback(result, "");
    });
  }

  std::string Resolve(const std::string& path) const override
  {
    return path;
  }
public:
  Scheduler scheduler;
};

AdblockPlus::FilterEnginePtr CreateFilterEngine(LazyFileSystem& fileSystem,
  AdblockPlus::Platform& platform,
  const AdblockPlus::FilterEngine::CreationParameters& creationParams = AdblockPlus::FilterEngine::CreationParameters());

class NoopWebRequest : public AdblockPlus::IWebRequest
{
public:
  void GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders, const GetCallback& callback) override
  {
  }
};

struct DelayedWebRequestTask
{
  std::string url;
  AdblockPlus::HeaderList headers;
  AdblockPlus::IWebRequest::GetCallback getCallback;
};

class DelayedWebRequest : public DelayedMixin<DelayedWebRequest, DelayedWebRequestTask, AdblockPlus::IWebRequest>
{
public:
  void GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders, const GetCallback& callback) override
  {
    Task task = { url, requestHeaders, callback };
    tasks->emplace_back(task);
  }
};

class LazyLogSystem : public AdblockPlus::LogSystem
{
public:
  void operator()(LogLevel logLevel, const std::string& message,
          const std::string& source)
  {
  }
};

struct ThrowingPlatformCreationParameters: AdblockPlus::Platform::CreationParameters
{
  ThrowingPlatformCreationParameters();
};

class BaseJsTest : public ::testing::Test
{
protected:
  std::unique_ptr<AdblockPlus::Platform> platform;

  void SetUp() override
  {
    platform.reset(new AdblockPlus::Platform(ThrowingPlatformCreationParameters()));
  }

  AdblockPlus::JsEngine& GetJsEngine()
  {
    if (!platform)
      throw std::runtime_error("Platform must be initialized");
    return platform->GetJsEngine();
  }

  void TearDown() override
  {
    if (platform)
      platform.reset();
  }
};

#endif
