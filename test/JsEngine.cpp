#include <ErrorCallback.h>
#include <FileReader.h>
#include <fstream>
#include <gtest/gtest.h>
#include <JsEngine.h>
#include <JsError.h>
#include <sstream>

struct ThrowingFileReader : public AdblockPlus::FileReader
{
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    throw std::runtime_error("Unexpected read of file: " + path);
  }
};

struct ThrowingErrorCallback : public AdblockPlus::ErrorCallback
{
  void operator()(const std::string& message)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

struct StubFileReader : public AdblockPlus::FileReader
{
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    std::stringstream* source = new std::stringstream;
    *source << "function hello() { return 'Hello'; }";
    return std::auto_ptr<std::istream>(source);
  }
};

struct BadFileReader : public AdblockPlus::FileReader
{
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    std::ifstream* file = new std::ifstream;
    file->open("");
    return std::auto_ptr<std::istream>(file);
  }
};

struct StubErrorCallback : public AdblockPlus::ErrorCallback
{
  std::string lastMessage;

  void operator()(const std::string& message)
  {
    lastMessage = message;
  }
};

TEST(JsEngineTest, EvaluateAndCall)
{
  ThrowingFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  const std::string source = "function hello() { return 'Hello'; }";
  jsEngine.Evaluate(source);
  const std::string result = jsEngine.Call("hello");
  ASSERT_EQ("Hello", result);
}

TEST(JsEngineTest, LoadAndCall)
{
  StubFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Load("hello.js");
  const std::string result = jsEngine.Call("hello");
  ASSERT_EQ("Hello", result);
}

TEST(JsEngineTest, LoadBadStreamFails)
{
  BadFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  ASSERT_ANY_THROW(jsEngine.Load("hello.js"));
}

TEST(JsEngineTest, JsExceptionIsThrown)
{
  ThrowingFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  ASSERT_THROW(jsEngine.Evaluate("doesnotexist()"), AdblockPlus::JsError);
}

TEST(JsEngineTest, ReportError)
{
  ThrowingFileReader fileReader;
  StubErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  jsEngine.Evaluate("reportError('fail')");
  ASSERT_EQ("fail", errorCallback.lastMessage);
}
