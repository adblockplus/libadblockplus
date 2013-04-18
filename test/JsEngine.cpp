#include <AdblockPlus.h>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>

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
  AdblockPlus::JsEngine jsEngine;
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  jsEngine.Evaluate("function hello() { return 'Hello'; }");
  AdblockPlus::JsValuePtr result = jsEngine.Evaluate("hello()");
  ASSERT_TRUE(result->IsString());
  ASSERT_EQ("Hello", result->AsString());
}

TEST(JsEngineTest, RuntimeExceptionIsThrown)
{
  AdblockPlus::JsEngine jsEngine;
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  ASSERT_THROW(jsEngine.Evaluate("doesnotexist()"), AdblockPlus::JsError);
}

TEST(JsEngineTest, CompileTimeExceptionIsThrown)
{
  AdblockPlus::JsEngine jsEngine;
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
  ASSERT_THROW(jsEngine.Evaluate("'foo'bar'"), AdblockPlus::JsError);
}

TEST(JsEngineTest, ValueCreation)
{
  AdblockPlus::JsEngine jsEngine;
  jsEngine.SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new ThrowingErrorCallback()));
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
