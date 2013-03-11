#include <AdblockPlus.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

class ThrowingFileReader : public AdblockPlus::FileReader
{
public:
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    throw std::runtime_error("Unexpected read of file: " + path);
  }
};

class ThrowingErrorCallback : public AdblockPlus::ErrorCallback
{
public:
  void operator()(const std::string& message)
  {
    throw std::runtime_error("Unexpected error: " + message);
  }
};

class StubFileReader : public AdblockPlus::FileReader
{
public:
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    std::stringstream* const source = new std::stringstream;
    *source << "function hello() { return 'Hello'; }";
    return std::auto_ptr<std::istream>(source);
  }
};

class BadFileReader : public AdblockPlus::FileReader
{
public:
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    std::ifstream* const file = new std::ifstream;
    file->open("");
    return std::auto_ptr<std::istream>(file);
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

TEST(JsEngineTest, RuntimeExceptionIsThrown)
{
  ThrowingFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  ASSERT_THROW(jsEngine.Evaluate("doesnotexist()"), AdblockPlus::JsError);
}

TEST(JsEngineTest, CompileTimeExceptionIsThrown)
{
  ThrowingFileReader fileReader;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileReader, &errorCallback);
  ASSERT_THROW(jsEngine.Evaluate("'foo'bar'"), AdblockPlus::JsError);
}
