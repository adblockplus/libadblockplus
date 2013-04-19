#ifndef MOCKS_H
#define MOCKS_H

#include <AdblockPlus.h>
#include <gtest/gtest.h>

class ThrowingErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  void operator()(const std::string& message)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

class ThrowingFileSystem : public AdblockPlus::FileSystem
{
  std::tr1::shared_ptr<std::istream> Read(const std::string& path) const
  {
    throw std::runtime_error("Not implemented");
  }

  void Write(const std::string& path,
             std::tr1::shared_ptr<std::ostream> content)
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
};

class ThrowingWebRequest : public AdblockPlus::WebRequest
{
  AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
  {
    throw std::runtime_error("Unexpected GET: " + url);
  }
};

class BaseJsTest : public ::testing::Test
{
protected:
  AdblockPlus::JsEnginePtr jsEngine;

  virtual void SetUp()
  {
    jsEngine = AdblockPlus::JsEngine::New();
    jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback));
    jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new ThrowingFileSystem));
    jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new ThrowingWebRequest));
  }
};

#endif
