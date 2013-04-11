#include <AdblockPlus.h>
#include <gtest/gtest.h>

#include "../src/Thread.h"

TEST(GlobalJsObjectTest, SetTimeout)
{
  AdblockPlus::JsEngine jsEngine(0, 0, 0);
  jsEngine.Evaluate("setTimeout(function() {foo = 'bar';}, 100)");
  ASSERT_EQ("undefined", jsEngine.Evaluate("typeof foo"));
  AdblockPlus::Sleep(200);
  ASSERT_EQ("bar", jsEngine.Evaluate("foo"));
}

TEST(GlobalJsObjectTest, SetTimeoutWithArgs)
{
  AdblockPlus::JsEngine jsEngine(0, 0, 0);
  jsEngine.Evaluate("setTimeout(function(s) {foo = s;}, 100, 'foobar')");
  ASSERT_EQ("undefined", jsEngine.Evaluate("typeof foo"));
  AdblockPlus::Sleep(200);
  ASSERT_EQ("foobar", jsEngine.Evaluate("foo"));
}

TEST(GlobalJsObjectTest, SetTimeoutWithInvalidArgs)
{
  AdblockPlus::JsEngine jsEngine(0, 0, 0);
  ASSERT_ANY_THROW(jsEngine.Evaluate("setTimeout()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("setTimeout('', 1)"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("setTimeout(function(){}, '')"));
}

TEST(GlobalJsObjectTest, SetMultipleTimeouts)
{
  AdblockPlus::JsEngine jsEngine(0, 0, 0);
  jsEngine.Evaluate("foo = []");
  jsEngine.Evaluate("setTimeout(function(s) {foo.push('1');}, 100)");
  jsEngine.Evaluate("setTimeout(function(s) {foo.push('2');}, 150)");
  AdblockPlus::Sleep(200);
  ASSERT_EQ("1,2", jsEngine.Evaluate("foo"));
}
