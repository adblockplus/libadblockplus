#include <AdblockPlus.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

class BaseFileSystem : public AdblockPlus::FileSystem
{
    void Write(const std::string& path,
               std::tr1::shared_ptr<std::ostream> content)
    {
      throw std::runtime_error("Write is not implemented");
    }

    void Move(const std::string& fromPath, const std::string& toPath)
    {
      throw std::runtime_error("Move is not implemented");
    }

    void Remove(const std::string& path)
    {
      throw std::runtime_error("Remove is not implemented");
    }

    StatResult Stat(const std::string& path) const
    {
      throw std::runtime_error("Stat is not implemented");
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

class StubFileSystem : public BaseFileSystem
{
public:
  std::tr1::shared_ptr<std::istream> Read(const std::string& path) const
  {
    std::stringstream* const source = new std::stringstream;
    *source << "function hello() { return 'Hello'; }";
    return std::tr1::shared_ptr<std::istream>(source);
  }
};

class BadFileSystem : public BaseFileSystem
{
public:
  std::tr1::shared_ptr<std::istream> Read(const std::string& path) const
  {
    std::ifstream* const file = new std::ifstream;
    file->open("");
    return std::tr1::shared_ptr<std::istream>(file);
  }

  void Write(const std::string& path,
             std::tr1::shared_ptr<std::ostream> content)
  {
    throw std::runtime_error("No writing");
  }
};

TEST(JsEngineTest, EvaluateAndCall)
{
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(0, 0, &errorCallback);
  const std::string source = "function hello() { return 'Hello'; }";
  jsEngine.Evaluate(source);
  AdblockPlus::JsValuePtr result = jsEngine.Evaluate("hello()");
  ASSERT_TRUE(result->IsString());
  ASSERT_EQ("Hello", result->AsString());
}

TEST(JsEngineTest, LoadAndCall)
{
  StubFileSystem fileSystem;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileSystem, 0, &errorCallback);
  jsEngine.Load("hello.js");
  AdblockPlus::JsValuePtr result = jsEngine.Evaluate("hello()");
  ASSERT_TRUE(result->IsString());
  ASSERT_EQ("Hello", result->AsString());
}

TEST(JsEngineTest, LoadBadStreamFails)
{
  BadFileSystem fileSystem;
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(&fileSystem, 0, &errorCallback);
  ASSERT_ANY_THROW(jsEngine.Load("hello.js"));
}

TEST(JsEngineTest, RuntimeExceptionIsThrown)
{
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(0, 0, &errorCallback);
  ASSERT_THROW(jsEngine.Evaluate("doesnotexist()"), AdblockPlus::JsError);
}

TEST(JsEngineTest, CompileTimeExceptionIsThrown)
{
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(0, 0, &errorCallback);
  ASSERT_THROW(jsEngine.Evaluate("'foo'bar'"), AdblockPlus::JsError);
}

TEST(JsEngineTest, ValueCreation)
{
  ThrowingErrorCallback errorCallback;
  AdblockPlus::JsEngine jsEngine(0, 0, &errorCallback);
  AdblockPlus::JsValuePtr value;

  value = jsEngine.NewValue("foo");
  ASSERT_TRUE(value->IsString());
  ASSERT_EQ("foo", value->AsString());

  value = jsEngine.NewValue(12);
  ASSERT_TRUE(value->IsNumber());
  ASSERT_EQ(12, value->AsInt());

  value = jsEngine.NewValue(true);
  ASSERT_TRUE(value->IsBool());
  ASSERT_TRUE(value->AsBool());

  value = jsEngine.NewObject();
  ASSERT_TRUE(value->IsObject());
  ASSERT_EQ(0u, value->GetOwnPropertyNames().size());
}
