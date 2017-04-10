/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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
#include <atomic>
#include <mutex>

using namespace AdblockPlus;

namespace
{
  class MockWebRequest : public AdblockPlus::WebRequest
  {
  public:
    AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
    {
      std::set<std::string> headerNames;
      for (auto header : requestHeaders)
      {
        headerNames.insert(header.first);
      }
      {
        std::lock_guard<std::mutex> lock(requestHeaderNamesMutex);
        // we currently ignore the result. We should check it actually gets inserted.
        requestHeaderNames.insert(std::make_pair(url, std::move(headerNames)));
      }

      AdblockPlus::Sleep(50);

      AdblockPlus::ServerResponse result;
      result.status = NS_OK;
      result.responseStatus = 123;
      result.responseHeaders.push_back(std::pair<std::string, std::string>("Foo", "Bar"));
      result.responseText = url + "\n";
      if (!requestHeaders.empty())
      {
        result.responseText += requestHeaders[0].first + "\n" + requestHeaders[0].second;
      }
      return result;
    }

    // Testing method
    // Get the headers for the request. Return a pair of a bool (found
    // or not) and the actual header names
    std::pair<bool, std::set<std::string>> headersForRequest(const std::string& url)
    {
      std::lock_guard<std::mutex> lock(requestHeaderNamesMutex);
      auto iter = requestHeaderNames.find(url);
      if (iter != requestHeaderNames.end())
      {
        auto result = std::make_pair(true, iter->second);
        requestHeaderNames.erase(iter);
        return result;
      }
      return std::make_pair(false, std::set<std::string>());
    }

    // mutable. Very Ugly. But we are testing and need to change this in GET which is const.
    mutable std::mutex requestHeaderNamesMutex;
    mutable std::map<std::string, std::set<std::string>> requestHeaderNames;
  };

  template<class T>
  class WebRequestTest : public BaseJsTest
  {
  protected:
    void SetUp()
    {
      BaseJsTest::SetUp();
      webRequest = std::make_shared<T>();
      jsEngine->SetWebRequest(webRequest);
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem()));
    }

    std::shared_ptr<T> webRequest;
  };

  typedef WebRequestTest<MockWebRequest> MockWebRequestTest;
  typedef WebRequestTest<AdblockPlus::DefaultWebRequest> DefaultWebRequestTest;
  typedef WebRequestTest<MockWebRequest> XMLHttpRequestTest;

  // we return the url of the XHR.
  std::string ResetTestXHR(const AdblockPlus::JsEnginePtr& jsEngine, const std::string& defaultUrl = "")
  {
    std::string url = defaultUrl;
    // make up a unique URL if we don't have one.
    if (url == "")
    {
      url = "https://tests.adblockplus.org/easylist.txt-";
      url += std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }

    jsEngine->Evaluate(std::string("\
      var result;\
      var request = new XMLHttpRequest();\
      request.open('GET', '") + url + "'); \
      request.overrideMimeType('text/plain');\
      request.addEventListener('load', function() {result = request.responseText;}, false);\
      request.addEventListener('error', function() {result = 'error';}, false);\
    ");
    return url;
  }

  void WaitForVariable(const std::string& variable, const AdblockPlus::JsEnginePtr& jsEngine)
  {
    do
    {
      AdblockPlus::Sleep(60);
    } while (jsEngine->Evaluate(variable)->IsUndefined());
  }

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
  jsEngine->Evaluate("_webRequest.GET('https://easylist-downloads.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  WaitForVariable("this.foo", jsEngine);
  ASSERT_EQ("text/plain", jsEngine->Evaluate("foo.responseHeaders['content-type'].substr(0, 10)")->AsString());
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(200, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("[Adblock Plus ", jsEngine->Evaluate("foo.responseText.substr(0, 14)")->AsString());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("foo.responseHeaders['content-type'].substr(0, 10)")->AsString());
#if defined(HAVE_CURL)
  ASSERT_EQ("gzip", jsEngine->Evaluate("foo.responseHeaders['content-encoding'].substr(0, 4)")->AsString());
#endif
  ASSERT_TRUE(jsEngine->Evaluate("foo.responseHeaders['location']")->IsUndefined());
}

TEST_F(DefaultWebRequestTest, XMLHttpRequest)
{
  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);

  ResetTestXHR(jsEngine, "https://easylist-downloads.adblockplus.org/easylist.txt");
  jsEngine->Evaluate("\
    request.setRequestHeader('X', 'Y');\
    request.setRequestHeader('X2', 'Y2');\
    request.send(null);");
  WaitForVariable("result", jsEngine);
  ASSERT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("request.channel.status")->AsInt());
  ASSERT_EQ(200, jsEngine->Evaluate("request.status")->AsInt());
  ASSERT_EQ("[Adblock Plus ", jsEngine->Evaluate("result.substr(0, 14)")->AsString());
  ASSERT_EQ("text/plain", jsEngine->Evaluate("request.getResponseHeader('Content-Type').substr(0, 10)")->AsString());
#if defined(HAVE_CURL)
  ASSERT_EQ("gzip", jsEngine->Evaluate("request.getResponseHeader('Content-Encoding').substr(0, 4)")->AsString());
#endif
  ASSERT_TRUE(jsEngine->Evaluate("request.getResponseHeader('Location')")->IsNull());
}
#else
TEST_F(DefaultWebRequestTest, DummyWebRequest)
{
  jsEngine->Evaluate("_webRequest.GET('https://easylist-downloads.adblockplus.org/easylist.txt', {}, function(result) {foo = result;} )");
  WaitForVariable("this.foo", jsEngine);
  ASSERT_EQ(AdblockPlus::WebRequest::NS_ERROR_FAILURE, jsEngine->Evaluate("foo.status")->AsInt());
  ASSERT_EQ(0, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  ASSERT_EQ("", jsEngine->Evaluate("foo.responseText")->AsString());
  ASSERT_EQ("{}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

TEST_F(DefaultWebRequestTest, XMLHttpRequest)
{
  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);

  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('X', 'Y');\
    request.send(null);");
  WaitForVariable("result", jsEngine);
  ASSERT_EQ(AdblockPlus::WebRequest::NS_ERROR_FAILURE, jsEngine->Evaluate("request.channel.status")->AsInt());
  ASSERT_EQ(0, jsEngine->Evaluate("request.status")->AsInt());
  ASSERT_EQ("error", jsEngine->Evaluate("result")->AsString());
  ASSERT_TRUE(jsEngine->Evaluate("request.getResponseHeader('Content-Type')")->IsNull());
}

#endif

namespace
{
  class CatchLogSystem : public AdblockPlus::LogSystem
  {
  public:
    AdblockPlus::LogSystem::LogLevel lastLogLevel;
    std::string lastMessage;

    CatchLogSystem()
      : AdblockPlus::LogSystem(),
        lastLogLevel(AdblockPlus::LogSystem::LOG_LEVEL_TRACE)
    {
    }

    void operator()(AdblockPlus::LogSystem::LogLevel logLevel,
        const std::string& message, const std::string&)
    {
      lastLogLevel = logLevel;
      lastMessage = message;
    }

    void clear()
    {
      lastLogLevel = AdblockPlus::LogSystem::LOG_LEVEL_TRACE;
      lastMessage.clear();
    }
  };

  typedef std::shared_ptr<CatchLogSystem> CatchLogSystemPtr;
}

TEST_F(XMLHttpRequestTest, RequestHeaderValidation)
{
  auto catchLogSystem = CatchLogSystemPtr(new CatchLogSystem());
  jsEngine->SetLogSystem(catchLogSystem);

  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);

  const std::string msg = "Attempt to set a forbidden header was denied: ";

  // The test will check that console.warn has been called when the
  // header is rejected. While this is an implementation detail, we
  // have no other way to check this

  // test 'Accept-Encoding' is rejected
  catchLogSystem->clear();
  std::string url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Accept-Encoding', 'gzip');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Accept-Encoding", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_TRUE(headers.cend() == headers.find("Accept-Encoding"));
  }

  // test 'DNT' is rejected
  catchLogSystem->clear();
  url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('DNT', '1');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "DNT", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_TRUE(headers.cend() == headers.find("DNT"));
  }

  // test random 'X' header is accepted
  catchLogSystem->clear();
  url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('X', 'y');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, catchLogSystem->lastLogLevel);
  EXPECT_EQ("", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_FALSE(headers.cend() == headers.find("X"));
  }

  // test /^Proxy-/ is rejected.
  catchLogSystem->clear();
  url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Proxy-foo', 'bar');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Proxy-foo", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_TRUE(headers.cend() == headers.find("Proxy-foo"));
  }

  // test /^Sec-/ is rejected.
  catchLogSystem->clear();
  url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Sec-foo', 'bar');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Sec-foo", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_TRUE(headers.cend() == headers.find("Sec-foo"));
  }

  // test 'Security' is accepted.
  catchLogSystem->clear();
  url = ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Security', 'theater');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, catchLogSystem->lastLogLevel);
  EXPECT_EQ("", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  {
    auto headersRequest = webRequest->headersForRequest(url);
    EXPECT_TRUE(headersRequest.first);
    const auto& headers = headersRequest.second;
    EXPECT_FALSE(headers.cend() == headers.find("Security"));
  }
}
