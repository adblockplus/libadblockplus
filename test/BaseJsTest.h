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

class LazyFileSystem : public AdblockPlus::IFileSystem, public AdblockPlus::FileSystem
{
public:
  IOBuffer Read(const std::string& path) const
  {
    std::string dummyData("");
    if (path == "patterns.ini")
      dummyData = "# Adblock Plus preferences\n[Subscription]\nurl=~fl~";
    else if (path == "prefs.json")
      dummyData = "{}";
    return IOBuffer(dummyData.cbegin(), dummyData.cend());
  }

  void Read(const std::string& path, const ReadCallback& callback) const
  {
    std::thread([this, path, callback]
    {
      auto data = Read(path);
      callback(std::move(data), "");
    }).detach();
  }

  void Write(const std::string& path, const IOBuffer& content)
  {
  }

  void Write(const std::string& path, const IOBuffer& data,
             const Callback& callback)
  {
    std::thread([this, path, data, callback]
    {
      Write(path, data);
      callback("");
    }).detach();
  }

  void Move(const std::string& fromPath, const std::string& toPath)
  {
  }

  void Move(const std::string& fromPath, const std::string& toPath,
            const Callback& callback)
  {
    std::thread([this, fromPath, toPath, callback]
    {
      Move(fromPath, toPath);
      callback("");
    }).detach();
  }

  void Remove(const std::string& path)
  {
  }

  void Remove(const std::string& path, const Callback& callback)
  {
    std::thread([this, path, callback]
    {
      Remove(path);
      callback("");
    }).detach();
  }

  StatResult Stat(const std::string& path) const
  {
    StatResult result;
    if (path == "patterns.ini")
    {
      result.exists = true;
      result.isFile = true;
    }
    return result;
  }

  void Stat(const std::string& path, const StatCallback& callback) const
  {
    std::thread([this, path, callback]
    {
      callback(Stat(path), "");
    }).detach();
  }

  std::string Resolve(const std::string& path) const
  {
    return path;
  }
};

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

struct JsEngineCreationParameters
{
  JsEngineCreationParameters();

  AdblockPlus::AppInfo appInfo;
  AdblockPlus::LogSystemPtr logSystem;
  AdblockPlus::TimerPtr timer;
  AdblockPlus::WebRequestPtr webRequest;
  AdblockPlus::FileSystemPtr fileSystem;
};

AdblockPlus::JsEnginePtr CreateJsEngine(JsEngineCreationParameters&& jsEngineCreationParameters = JsEngineCreationParameters());

class BaseJsTest : public ::testing::Test
{
protected:
  AdblockPlus::JsEnginePtr jsEngine;

  virtual void SetUp()
  {
    jsEngine = CreateJsEngine();
  }
};

#endif
