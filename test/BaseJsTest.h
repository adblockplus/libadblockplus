/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include "../src/Thread.h"

class ThrowingLogSystem : public AdblockPlus::LogSystem
{
public:
  void operator()(LogLevel logLevel, const std::string& message,
        const std::string& source)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

class ThrowingFileSystem : public AdblockPlus::FileSystem
{
public:
  std::shared_ptr<std::istream> Read(const std::string& path) const
  {
    throw std::runtime_error("Not implemented");
  }

  void Write(const std::string& path, std::shared_ptr<std::istream> content)
  {
    throw std::runtime_error("Not implemented");
  }

  void Move(const std::string& fromPath, const std::string& toPath)
  {
    throw std::runtime_error("Not implemented");
  }

  void Remove(const std::string& path)
  {
    throw std::runtime_error("Not implemented");
  }

  StatResult Stat(const std::string& path) const
  {
    throw std::runtime_error("Not implemented");
  }

  std::string Resolve(const std::string& path) const
  {
    throw std::runtime_error("Not implemented");
  }

};

class ThrowingWebRequest : public AdblockPlus::WebRequest
{
public:
  AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
  {
    throw std::runtime_error("Unexpected GET: " + url);
  }
};

class LazyFileSystem : public AdblockPlus::FileSystem
{
public:
  std::shared_ptr<std::istream> Read(const std::string& path) const
  {
    std::string dummyData("");
    if (path == "patterns.ini")
      dummyData = "# Adblock Plus preferences\n[Subscription]\nurl=~fl~";
    else if (path == "prefs.json")
      dummyData = "{}";
    return std::shared_ptr<std::istream>(new std::istringstream(dummyData));
  }

  void Write(const std::string& path, std::shared_ptr<std::istream> content)
  {
  }

  void Move(const std::string& fromPath, const std::string& toPath)
  {
  }

  void Remove(const std::string& path)
  {
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

  std::string Resolve(const std::string& path) const
  {
    return path;
  }
};

class LazyWebRequest : public AdblockPlus::WebRequest
{
public:
  AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
  {
    while (true)
      AdblockPlus::Sleep(100000);
    return AdblockPlus::ServerResponse();
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

AdblockPlus::JsEnginePtr CreateJsEngine(const AdblockPlus::AppInfo& appInfo = AdblockPlus::AppInfo());

class BaseJsTest : public ::testing::Test
{
protected:
  AdblockPlus::JsEnginePtr jsEngine;

  virtual void SetUp()
  {
    jsEngine = CreateJsEngine();
    jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new ThrowingLogSystem));
    jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new ThrowingFileSystem));
    jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new ThrowingWebRequest));
  }
};

#endif
