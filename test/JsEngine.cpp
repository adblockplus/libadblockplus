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

TEST(JsEngineTest, Evaluate)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  jsEngine->Evaluate("function hello() { return 'Hello'; }");
  AdblockPlus::JsValuePtr result = jsEngine->Evaluate("hello()");
  ASSERT_TRUE(result->IsString());
  ASSERT_EQ("Hello", result->AsString());
}

TEST(JsEngineTest, RuntimeExceptionIsThrown)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  ASSERT_THROW(jsEngine->Evaluate("doesnotexist()"), AdblockPlus::JsError);
}

TEST(JsEngineTest, CompileTimeExceptionIsThrown)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  ASSERT_THROW(jsEngine->Evaluate("'foo'bar'"), AdblockPlus::JsError);
}

TEST(JsEngineTest, ValueCreation)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  AdblockPlus::JsValuePtr value;

  value = jsEngine->NewValue("foo");
  ASSERT_TRUE(value->IsString());
  ASSERT_EQ("foo", value->AsString());

  value = jsEngine->NewValue(12345678901234);
  ASSERT_TRUE(value->IsNumber());
  ASSERT_EQ(12345678901234, value->AsInt());

  value = jsEngine->NewValue(true);
  ASSERT_TRUE(value->IsBool());
  ASSERT_TRUE(value->AsBool());

  value = jsEngine->NewObject();
  ASSERT_TRUE(value->IsObject());
  ASSERT_EQ(0u, value->GetOwnPropertyNames().size());
}

TEST(JsEngineTest, CallbackGetSet)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());

  ASSERT_TRUE(jsEngine->GetErrorCallback());
  ASSERT_ANY_THROW(jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr()));
  AdblockPlus::ErrorCallbackPtr errorCallback(new AdblockPlus::DefaultErrorCallback());
  jsEngine->SetErrorCallback(errorCallback);
  ASSERT_EQ(errorCallback, jsEngine->GetErrorCallback());

  ASSERT_TRUE(jsEngine->GetFileSystem());
  ASSERT_ANY_THROW(jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr()));
  AdblockPlus::FileSystemPtr fileSystem(new AdblockPlus::DefaultFileSystem());
  jsEngine->SetFileSystem(fileSystem);
  ASSERT_EQ(fileSystem, jsEngine->GetFileSystem());

  ASSERT_TRUE(jsEngine->GetWebRequest());
  ASSERT_ANY_THROW(jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr()));
  AdblockPlus::WebRequestPtr webRequest(new AdblockPlus::DefaultWebRequest());
  jsEngine->SetWebRequest(webRequest);
  ASSERT_EQ(webRequest, jsEngine->GetWebRequest());
}
