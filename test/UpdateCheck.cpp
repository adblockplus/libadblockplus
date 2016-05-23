/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#include <functional>

#include "BaseJsTest.h"

namespace
{
  typedef std::shared_ptr<AdblockPlus::FilterEngine> FilterEnginePtr;

  void FindAndReplace(std::string& source, const std::string& find, const std::string& replace)
  {
    for (size_t pos = 0; (pos = source.find(find), pos) != std::string::npos; pos += replace.size())
      source.replace(pos, find.size(), replace);
  }

  std::string previousRequestUrl;
  class TestWebRequest : public LazyWebRequest
  {
  public:
    AdblockPlus::ServerResponse response;
    AdblockPlus::ServerResponse GET(const std::string& url, const AdblockPlus::HeaderList& requestHeaders) const
    {
      if (url.find("easylist") != std::string::npos)
        return LazyWebRequest::GET(url, requestHeaders);

      previousRequestUrl = url;
      return response;
    }
  };

  class UpdateCheckTest : public ::testing::Test
  {
  protected:
    AdblockPlus::AppInfo appInfo;
    TestWebRequest* webRequest;
    AdblockPlus::WebRequestPtr webRequestPtr;
    AdblockPlus::JsEnginePtr jsEngine;
    FilterEnginePtr filterEngine;

    bool eventCallbackCalled;
    AdblockPlus::JsValueList eventCallbackParams;
    bool updateCallbackCalled;
    std::string updateError;

    void SetUp()
    {
      webRequest = new TestWebRequest();
      webRequestPtr.reset(webRequest);

      eventCallbackCalled = false;
      updateCallbackCalled = false;
      Reset();
    }

    void Reset()
    {
      jsEngine = CreateJsEngine(appInfo);
      jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new LazyLogSystem));
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem));
      jsEngine->SetWebRequest(webRequestPtr);
      jsEngine->SetEventCallback("updateAvailable",
          std::bind(&UpdateCheckTest::EventCallback, this, std::placeholders::_1));

      filterEngine.reset(new AdblockPlus::FilterEngine(jsEngine));
    }

    void ForceUpdateCheck()
    {
      filterEngine->ForceUpdateCheck(
          std::bind(&UpdateCheckTest::UpdateCallback, this, std::placeholders::_1));
    }

    void EventCallback(AdblockPlus::JsValueList& params)
    {
      eventCallbackCalled = true;
      eventCallbackParams = params;
    }

    void UpdateCallback(const std::string& error)
    {
      updateCallbackCalled = true;
      updateError = error;
    }
  };
}

TEST_F(UpdateCheckTest, RequestFailure)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_ERROR_FAILURE;

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = false;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_FALSE(updateError.empty());

  std::string expectedUrl(filterEngine->GetPref("update_url_release")->AsString());
  std::string platform = jsEngine->Evaluate("require('info').platform")->AsString();
  std::string platformVersion = jsEngine->Evaluate("require('info').platformVersion")->AsString();

  FindAndReplace(expectedUrl, "%NAME%", appInfo.name);
  FindAndReplace(expectedUrl, "%TYPE%", "1");   // manual update
  expectedUrl += "&addonName=" + appInfo.name +
                 "&addonVersion=" + appInfo.version +
                 "&application=" + appInfo.application +
                 "&applicationVersion=" + appInfo.applicationVersion +
                 "&platform=" + platform +
                 "&platformVersion=" + platformVersion +
                 "&lastVersion=0&downloadCount=0";
  ASSERT_EQ(expectedUrl, previousRequestUrl);
}

TEST_F(UpdateCheckTest, UpdateAvailable)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_OK;
  webRequest->response.responseStatus = 200;
  webRequest->response.responseText = "{\"1\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_TRUE(eventCallbackCalled);
  ASSERT_EQ(1u, eventCallbackParams.size());
  ASSERT_EQ("https://foo.bar/", eventCallbackParams[0]->AsString());
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());

  std::string expectedUrl(filterEngine->GetPref("update_url_devbuild")->AsString());
  std::string platform = jsEngine->Evaluate("require('info').platform")->AsString();
  std::string platformVersion = jsEngine->Evaluate("require('info').platformVersion")->AsString();

  FindAndReplace(expectedUrl, "%NAME%", appInfo.name);
  FindAndReplace(expectedUrl, "%TYPE%", "1");   // manual update
  expectedUrl += "&addonName=" + appInfo.name +
                 "&addonVersion=" + appInfo.version +
                 "&application=" + appInfo.application +
                 "&applicationVersion=" + appInfo.applicationVersion +
                 "&platform=" + platform +
                 "&platformVersion=" + platformVersion +
                 "&lastVersion=0&downloadCount=0";
  ASSERT_EQ(expectedUrl, previousRequestUrl);
}

TEST_F(UpdateCheckTest, ApplicationUpdateAvailable)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_OK;
  webRequest->response.responseStatus = 200;
  webRequest->response.responseText = "{\"1/4\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_TRUE(eventCallbackCalled);
  ASSERT_EQ(1u, eventCallbackParams.size());
  ASSERT_EQ("https://foo.bar/", eventCallbackParams[0]->AsString());
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongApplication)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_OK;
  webRequest->response.responseStatus = 200;
  webRequest->response.responseText = "{\"1/3\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongVersion)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_OK;
  webRequest->response.responseStatus = 200;
  webRequest->response.responseText = "{\"1\": {\"version\":\"3\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongURL)
{
  webRequest->response.status = AdblockPlus::WebRequest::NS_OK;
  webRequest->response.responseStatus = 200;
  webRequest->response.responseText = "{\"1\": {\"version\":\"3.1\",\"url\":\"http://insecure/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  Reset();
  ForceUpdateCheck();

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_FALSE(updateError.empty());
}
