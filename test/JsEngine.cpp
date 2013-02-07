#include <JsEngine.h>
#include <gtest/gtest.h>
#include <sstream>

TEST(JsEngineTest, EvaluateAndCall)
{
  AdblockPlus::JsEngine jsEngine;
  const std::string source = "function hello() { return 'Hello'; }";
  jsEngine.Evaluate(source);
  const std::string result = jsEngine.Call("hello");
  EXPECT_EQ("Hello", result);
}
