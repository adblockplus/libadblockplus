#include <AdblockPlus.h>
#include <gtest/gtest.h>

#include "../src/Thread.h"

class TestWebRequest : public AdblockPlus::WebRequest
{
public:
  AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
  {
    AdblockPlus::Sleep(50);

    AdblockPlus::ServerResponse result;
    result.responseStatus = 123;
    result.responseHeaders.push_back(std::pair<std::string, std::string>("Foo", "Bar"));
    result.responseText = url + "\n" + requestHeaders[0].first + "\n" + requestHeaders[0].second;
    return result;
  }
};

TEST(WebRequestTest, BadCall)
{
  AdblockPlus::JsEngine jsEngine(0, 0, 0);
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET()"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET('', {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET({toString: false}, {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET('http://example.com/', null, function(){})"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET('http://example.com/', {}, null)"));
  ASSERT_ANY_THROW(jsEngine.Evaluate("_webRequest.GET('http://example.com/', {}, function(){}, 0)"));
}

TEST(WebRequestTest, UrlLoad)
{
  TestWebRequest webRequest;
  AdblockPlus::JsEngine jsEngine(0, &webRequest, 0);
  jsEngine.Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_EQ("undefined", jsEngine.Evaluate("typeof foo"));
  AdblockPlus::Sleep(200);
  ASSERT_EQ("123", jsEngine.Evaluate("foo.responseStatus"));
  ASSERT_EQ("http://example.com/\nX\nY", jsEngine.Evaluate("foo.responseText"));
  ASSERT_EQ("{\"Foo\":\"Bar\"}", jsEngine.Evaluate("JSON.stringify(foo.responseHeaders)"));
}
