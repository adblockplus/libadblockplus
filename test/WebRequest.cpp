/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include "BaseJsTest.h"
#include "../src/Thread.h"

namespace
{
  class MockWebRequest : public AdblockPlus::WebRequest
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

  template<class T>
  class WebRequestTest : public BaseJsTest
  {
  protected:
    void SetUp()
    {
      BaseJsTest::SetUp();
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new T));
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem));
    }
  };

  typedef WebRequestTest<MockWebRequest> MockWebRequestTest;
  typedef WebRequestTest<AdblockPlus::DefaultWebRequest> DefaultWebRequestTest;
}

TEST_F(MockWebRequestTest, BadCall)
{
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET()"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('', {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET({toString: false}, {}, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', null, function(){})"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', {}, null)"));
  ASSERT_ANY_THROW(jsEngine->Evaluate("_webRequest.GET('http://example.com/', {}, function(){}, 0)"));
}

TEST_F(MockWebRequestTest, SuccessfulRequest)
{
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  ASSERT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

#if defined(HAVE_CURL) || defined(_WIN32)
TEST_F(DefaultWebRequestTest, RealWebRequest)
{
  // This URL should redirect to easylist-downloads.adblockplus.org and we
  // should get the actual filter list back.
  jsEngine->Evaluate("_webRequest.GET('https://easylist.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  do
  {
    AdblockPlus::Sleep(200);
  } while (jsEngine->Evaluate("this.foo")->IsUndefined());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("foo.responseHeaders['content-type'].substr(0, 10)")->AsString());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(200, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("[Adblock Plus ", jsEngine->Evaluate("foo.responseText.substr(0, 14)")->AsString());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("foo.responseHeaders['content-type'].substr(0, 10)")->AsString());
  ASSERT_TRUE(jsEngine->Evaluate("foo.responseHeaders['location']")->IsUndefined());
}

TEST_F(DefaultWebRequestTest, XMLHttpRequest)
{
  AdblockPlus::FilterEngine filterEngine(jsEngine);

  jsEngine->Evaluate("\
    var result;\
    var request = new XMLHttpRequest();\
    request.open('GET', 'https://easylist.adblockplus.org/easylist.txt');\
    request.setRequestHeader('X', 'Y');\
    request.setRequestHeader('X2', 'Y2');\
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
#else
TEST_F(DefaultWebRequestTest, DummyWebRequest)
{
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

TEST_F(DefaultWebRequestTest, XMLHttpRequest)
{
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
