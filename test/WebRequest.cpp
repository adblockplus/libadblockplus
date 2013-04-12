#include <sstream>
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
    result.status = NS_OK;
    result.responseStatus = 123;
    result.responseHeaders.push_back(std::pair<std::string, std::string>("Foo", "Bar"));
    result.responseText = url + "\n" + requestHeaders[0].first + "\n" + requestHeaders[0].second;
    return result;
  }
};

std::string ToString(unsigned int i)
{
  std::stringstream stream;
  stream << i;
  return stream.str();
}

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

TEST(WebRequestTest, TestWebRequest)
{
  TestWebRequest webRequest;
  AdblockPlus::JsEngine jsEngine(0, &webRequest, 0);
  jsEngine.Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_EQ("undefined", jsEngine.Evaluate("typeof foo"));
  AdblockPlus::Sleep(200);
  ASSERT_EQ(ToString(AdblockPlus::WebRequest::NS_OK), jsEngine.Evaluate("foo.status"));
  ASSERT_EQ("123", jsEngine.Evaluate("foo.responseStatus"));
  ASSERT_EQ("http://example.com/\nX\nY", jsEngine.Evaluate("foo.responseText"));
  ASSERT_EQ("{\"Foo\":\"Bar\"}", jsEngine.Evaluate("JSON.stringify(foo.responseHeaders)"));
}

#if defined(HAVE_CURL)
TEST(WebRequestTest, RealWebRequest)
{
  AdblockPlus::DefaultWebRequest webRequest;
  AdblockPlus::JsEngine jsEngine(0, &webRequest, 0);

  // This URL should redirect to easylist-downloads.adblockplus.org and we
  // should get the actual filter list back.
  jsEngine.Evaluate("_webRequest.GET('https://easylist.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine.Evaluate("typeof foo") == "undefined");
  ASSERT_EQ(ToString(AdblockPlus::WebRequest::NS_OK), jsEngine.Evaluate("foo.status"));
  ASSERT_EQ("200", jsEngine.Evaluate("foo.responseStatus"));
  ASSERT_EQ("[Adblock Plus ", jsEngine.Evaluate("foo.responseText.substr(0, 14)"));
  ASSERT_EQ("text/plain", jsEngine.Evaluate("foo.responseHeaders['content-type'].substr(0, 10)"));
  ASSERT_EQ("undefined", jsEngine.Evaluate("typeof foo.responseHeaders['location']"));
}
#else
TEST(WebRequestTest, DummyWebRequest)
{
  AdblockPlus::DefaultWebRequest webRequest;
  AdblockPlus::JsEngine jsEngine(0, &webRequest, 0);
  jsEngine.Evaluate("_webRequest.GET('https://easylist.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine.Evaluate("typeof foo") == "undefined");
  ASSERT_EQ(ToString(AdblockPlus::WebRequest::NS_ERROR_FAILURE), jsEngine.Evaluate("foo.status"));
  ASSERT_EQ("0", jsEngine.Evaluate("foo.responseStatus"));
  ASSERT_EQ("", jsEngine.Evaluate("foo.responseText"));
  ASSERT_EQ("{}", jsEngine.Evaluate("JSON.stringify(foo.responseHeaders)"));
}
#endif
