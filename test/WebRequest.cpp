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
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('', {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET({toString: false}, {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', null, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', {}, null)"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', {}, function(){}, 0)"));
}

TEST(WebRequestTest, TestWebRequest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new TestWebRequest()));
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  ASSERT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

#if defined(HAVE_CURL) or defined(HAVE_WININET)
TEST(WebRequestTest, RealWebRequest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());

  // This URL should redirect to easylist-downloads.adblockplus.org and we
  // should get the actual filter list back.
  jsEngine->Evaluate("_webRequest.GET('https://easylist.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine->Evaluate("this.foo")->IsUndefined());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(200, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("[Adblock Plus ", jsEngine->Evaluate("foo.responseText.substr(0, 14)")->AsString());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("foo.responseHeaders['content-type'].substr(0, 10)")->AsString());
  ASSERT_TRUE(jsEngine->Evaluate("foo.responseHeaders['location']")->IsUndefined());
}

#if !FILTER_ENGINE_STUBS
TEST(WebRequestTest, XMLHttpRequest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  AdblockPlus::FilterEngine filterEngine(jsEngine);

  jsEngine->Evaluate("\
    var result;\
    var request = new XMLHttpRequest();\
    request.open('GET', 'https://easylist.adblockplus.org/easylist.txt');\
    request.setRequestHeader('X', 'Y');\
    request.overrideMimeType('text/plain');\
    request.addEventListener('load', function() {result = request.responseText;}, false);\
    request.addEventListener('error', function() {result = 'error';}, false);\
    request.send(null);");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine->Evaluate("result")->IsUndefined());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("request.channel.status")->AsInt());
  ASSERT_EQ(200, jsEngine->Evaluate("request.status")->AsInt());
  ASSERT_EQ("[Adblock Plus ", jsEngine->Evaluate("result.substr(0, 14)")->AsString());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("request.getResponseHeader('Content-Type').substr(0, 10)")->AsString());
  ASSERT_TRUE(jsEngine->Evaluate("request.getResponseHeader('Location')")->IsNull());
}
#endif

#else
TEST(WebRequestTest, DummyWebRequest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  jsEngine->Evaluate("_webRequest.GET('https://easylist.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine->Evaluate("this.foo")->IsUndefined());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_ERROR_FAILURE, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(0, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("", jsEngine->Evaluate("foo.responseText")->AsString());
  ASSERT_EQ("{}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

#if !FILTER_ENGINE_STUBS
TEST(WebRequestTest, XMLHttpRequest)
{
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New());
  AdblockPlus::FilterEngine filterEngine(jsEngine);

  jsEngine->Evaluate("\
    var result;\
    var request = new XMLHttpRequest();\
    request.open('GET', 'https://easylist.adblockplus.org/easylist.txt');\
    request.setRequestHeader('X', 'Y');\
    request.overrideMimeType('text/plain');\
    request.addEventListener('load', function() {result = request.responseText;}, false);\
    request.addEventListener('error', function() {result = 'error';}, false);\
    request.send(null);");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine->Evaluate("result")->IsUndefined());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_ERROR_FAILURE, jsEngine->Evaluate("request.channel.status")->AsInt());
  ASSERT_EQ(0, jsEngine->Evaluate("request.status")->AsInt());
  ASSERT_EQ("error", jsEngine->Evaluate("result")->AsString());
  ASSERT_TRUE(jsEngine->Evaluate("request.getResponseHeader('Content-Type')")->IsNull());
}
#endif

#endif
