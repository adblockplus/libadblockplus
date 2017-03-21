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
      lastRequestHeaders.clear();
      for (auto header : requestHeaders)
      {
        lastRequestHeaders.insert(header.first);
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

    // mutable. Very Ugly. But we are testing and need to change this in GET which is const.
    mutable std::set<std::string> lastRequestHeaders;
  };

  template<class T>
  class WebRequestTest : public BaseJsTest
  {
  protected:
    void SetUp()
    {
      BaseJsTest::SetUp();
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new T()));
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem()));
    }
  };

  typedef WebRequestTest<MockWebRequest> MockWebRequestTest;
  typedef WebRequestTest<AdblockPlus::DefaultWebRequest> DefaultWebRequestTest;
  typedef WebRequestTest<MockWebRequest> XMLHttpRequestTest;

  void ResetTestXHR(const AdblockPlus::JsEnginePtr& jsEngine)
  {
    jsEngine->Evaluate("\
      var result;\
      var request = new XMLHttpRequest();\
      request.open('GET', 'https://easylist-downloads.adblockplus.org/easylist.txt');\
      request.overrideMimeType('text/plain');\
      request.addEventListener('load', function() {result = request.responseText;}, false);\
      request.addEventListener('error', function() {result = 'error';}, false);\
    ");
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

TEST_F(MockWebRequestTest, ConnectionIsAllowedOnJsEngine)
{
  std::atomic<int> isConnectionAllowedCalledTimes(0);
  jsEngine->SetIsConnectionAllowedCallback([&isConnectionAllowedCalledTimes]()->bool
  {
    ++isConnectionAllowedCalledTimes;
    return true;
  });
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  EXPECT_EQ(1u, isConnectionAllowedCalledTimes);
  EXPECT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

TEST_F(MockWebRequestTest, ConnectionIsNotAllowedOnJsEngine)
{
  std::atomic<int> isConnectionAllowedCalledTimes(0);
  jsEngine->SetIsConnectionAllowedCallback([&isConnectionAllowedCalledTimes]()->bool
  {
    ++isConnectionAllowedCalledTimes;
    return false;
  });
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  EXPECT_EQ(1u, isConnectionAllowedCalledTimes);
  EXPECT_EQ(AdblockPlus::WebRequest::NS_ERROR_CONNECTION_REFUSED, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(0, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

namespace
{
  class SyncStrings
  {
  public:
    void Add(const std::string* value)
    {
      std::lock_guard<std::mutex> lock(mutex);
      strings.emplace_back(!!value, value ? *value : "");
    }
    std::vector<std::pair<bool, std::string>> GetStrings() const
    {
      std::lock_guard<std::mutex> lock(mutex);
      return strings;
    }
    void Clear()
    {
      std::lock_guard<std::mutex> lock(mutex);
      strings.clear();
    }
  private:
    mutable std::mutex mutex;
    std::vector<std::pair<bool, std::string>> strings;
  };
}

TEST_F(MockWebRequestTest, ConnectionIsAllowedOnFilterEngine1)
{
  FilterEngine::CreationParameters createParams;
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.emplace("allowed_connection_type", jsEngine->NewValue(predefinedAllowedConnectionType));
  auto receivedConnectionTypes = std::make_shared<SyncStrings>();
  createParams.isConnectionAllowedCallback = [receivedConnectionTypes](const std::string* allowedConnectionType)->bool {
    receivedConnectionTypes->Add(allowedConnectionType);
    return true;
  };
  auto filterEngine = FilterEngine::Create(jsEngine, createParams);
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  auto receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_TRUE(connectionType.first);
    EXPECT_EQ(predefinedAllowedConnectionType, connectionType.second);
  }
  EXPECT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

TEST_F(MockWebRequestTest, ConnectionIsAllowedOnFilterEngine2)
{
  FilterEngine::CreationParameters createParams;
  auto receivedConnectionTypes = std::make_shared<SyncStrings>();
  createParams.isConnectionAllowedCallback = [receivedConnectionTypes](const std::string* allowedConnectionType)->bool {
    receivedConnectionTypes->Add(allowedConnectionType);
    return true;
  };
  auto filterEngine = FilterEngine::Create(jsEngine, createParams);
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  auto receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_FALSE(connectionType.first);
  }
  EXPECT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
}

TEST_F(MockWebRequestTest, ConnectionIsAllowedOnFilterEngine3)
{
  // initially allowed connection type is not defined
  FilterEngine::CreationParameters createParams;
  auto receivedConnectionTypes = std::make_shared<SyncStrings>();
  createParams.isConnectionAllowedCallback = [receivedConnectionTypes](const std::string* allowedConnectionType)->bool {
    receivedConnectionTypes->Add(allowedConnectionType);
    return true;
  };
  auto filterEngine = FilterEngine::Create(jsEngine, createParams);

  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  auto receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_FALSE(connectionType.first);
  }
  EXPECT_EQ(AdblockPlus::WebRequest::NS_OK, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(123, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("http://example.com/\nX\nY", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{\"Foo\":\"Bar\"}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());

  // set allowed connection type
  std::string allowedConnectionType = "test-connection";
  filterEngine->SetAllowedConnectionType(&allowedConnectionType);
  receivedConnectionTypes->Clear();
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  AdblockPlus::Sleep(200);
  receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_TRUE(connectionType.first);
    EXPECT_EQ(allowedConnectionType, connectionType.second);
  }

  // remove allowed connection type
  filterEngine->SetAllowedConnectionType(nullptr);
  receivedConnectionTypes->Clear();
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  AdblockPlus::Sleep(200);
  receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_FALSE(connectionType.first);
  }
}

TEST_F(MockWebRequestTest, ConnectionIsNotAllowedOnFilterEngine)
{
  FilterEngine::CreationParameters createParams;
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.emplace("allowed_connection_type", jsEngine->NewValue(predefinedAllowedConnectionType));
  auto receivedConnectionTypes = std::make_shared<SyncStrings>();
  createParams.isConnectionAllowedCallback = [receivedConnectionTypes](const std::string* allowedConnectionType)->bool {
    receivedConnectionTypes->Add(allowedConnectionType);
    return false;
  };
  auto filterEngine = FilterEngine::Create(jsEngine, createParams);
  jsEngine->Evaluate("_webRequest.GET('http://example.com/', {X: 'Y'}, function(result) {foo = result;} )");
  ASSERT_TRUE(jsEngine->Evaluate("this.foo")->IsUndefined());
  AdblockPlus::Sleep(200);
  auto receivedConnectionTypesStrings = receivedConnectionTypes->GetStrings();
  EXPECT_FALSE(receivedConnectionTypesStrings.empty());
  for (const auto& connectionType : receivedConnectionTypesStrings)
  {
    EXPECT_TRUE(connectionType.first);
    EXPECT_EQ(predefinedAllowedConnectionType, connectionType.second);
  }
  EXPECT_EQ(AdblockPlus::WebRequest::NS_ERROR_CONNECTION_REFUSED, jsEngine->Evaluate("foo.status")->AsInt());
  EXPECT_EQ(0, jsEngine->Evaluate("foo.responseStatus")->AsInt());
  EXPECT_EQ("", jsEngine->Evaluate("foo.responseText")->AsString());
  EXPECT_EQ("{}", jsEngine->Evaluate("JSON.stringify(foo.responseHeaders)")->AsString());
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

  ResetTestXHR(jsEngine);
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
  auto webRequest =
    std::static_pointer_cast<MockWebRequest>(jsEngine->GetWebRequest());

  ASSERT_TRUE(webRequest);

  const std::string msg = "Attempt to set a forbidden header was denied: ";

  // The test will check that console.warn has been called when the
  // header is rejected. While this is an implementation detail, we
  // have no other way to check this

  // test 'Accept-Encoding' is rejected
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Accept-Encoding', 'gzip');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Accept-Encoding", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_TRUE(webRequest->lastRequestHeaders.cend() ==
              webRequest->lastRequestHeaders.find("Accept-Encoding"));

  // test 'DNT' is rejected
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('DNT', '1');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "DNT", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_TRUE(webRequest->lastRequestHeaders.cend() ==
              webRequest->lastRequestHeaders.find("DNT"));

  // test random 'X' header is accepted
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('X', 'y');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, catchLogSystem->lastLogLevel);
  EXPECT_EQ("", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_FALSE(webRequest->lastRequestHeaders.cend() ==
               webRequest->lastRequestHeaders.find("X"));

  // test /^Proxy-/ is rejected.
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Proxy-foo', 'bar');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Proxy-foo", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_TRUE(webRequest->lastRequestHeaders.cend() ==
              webRequest->lastRequestHeaders.find("Proxy-foo"));

  // test /^Sec-/ is rejected.
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Sec-foo', 'bar');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_WARN, catchLogSystem->lastLogLevel);
  EXPECT_EQ(msg + "Sec-foo", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_TRUE(webRequest->lastRequestHeaders.cend() ==
              webRequest->lastRequestHeaders.find("Sec-foo"));

  // test 'Security' is accepted.
  catchLogSystem->clear();
  ResetTestXHR(jsEngine);
  jsEngine->Evaluate("\
    request.setRequestHeader('Security', 'theater');\nrequest.send();");
  EXPECT_EQ(AdblockPlus::LogSystem::LOG_LEVEL_TRACE, catchLogSystem->lastLogLevel);
  EXPECT_EQ("", catchLogSystem->lastMessage);
  WaitForVariable("result", jsEngine);
  EXPECT_FALSE(webRequest->lastRequestHeaders.cend() ==
               webRequest->lastRequestHeaders.find("Security"));
}
