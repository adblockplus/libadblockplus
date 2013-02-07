#include <FileReader.h>
#include <JsEngine.h>
#include <gtest/gtest.h>
#include <sstream>

class StubFileReader : public AdblockPlus::FileReader
{
  std::auto_ptr<std::istream> Read(const std::string& path) const
  {
    std::stringstream* source = new std::stringstream;
    *source << "function hello() { return 'Hello'; }";
    return std::auto_ptr<std::istream>(source);
  }
};

TEST(JsEngineTest, EvaluateAndCall)
{
  AdblockPlus::JsEngine jsEngine;
  const std::string source = "function hello() { return 'Hello'; }";
  jsEngine.Evaluate(source);
  const std::string result = jsEngine.Call("hello");
  EXPECT_EQ("Hello", result);
}

TEST(JsEngineTest, LoadAndCall)
{
  StubFileReader fileReader;
  AdblockPlus::JsEngine jsEngine;
  jsEngine.fileReader = &fileReader;
  jsEngine.Load("hello.js");
  const std::string result = jsEngine.Call("hello");
  EXPECT_EQ("Hello", result);
}
