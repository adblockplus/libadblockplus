/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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

using namespace AdblockPlus;

namespace
{
  void FindAndReplace(std::string& source, const std::string& find, const std::string& replace)
  {
    for (size_t pos = 0; (pos = source.find(find), pos) != std::string::npos; pos += replace.size())
      source.replace(pos, find.size(), replace);
  }

  class UpdateCheckTest : public BaseJsTest
  {
  protected:
    AdblockPlus::AppInfo appInfo;
    AdblockPlus::ServerResponse webRequestResponse;
    DelayedWebRequest::SharedTasks webRequestTasks;
    DelayedTimer::SharedTasks timerTasks;

    bool eventCallbackCalled;
    AdblockPlus::JsValueList eventCallbackParams;
    bool updateCallbackCalled;
    std::string updateError;

    void SetUp()
    {
      eventCallbackCalled = false;
      updateCallbackCalled = false;
    }

    void CreateUpdater()
    {
      LazyFileSystem* fileSystem;
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer = DelayedTimer::New(timerTasks);
      platformParams.fileSystem.reset(fileSystem = new LazyFileSystem());
      platformParams.webRequest = DelayedWebRequest::New(webRequestTasks);
      platform.reset(new Platform(std::move(platformParams)));
      platform->SetUpJsEngine(appInfo);
      GetJsEngine().SetEventCallback("updateAvailable", [this](JsValueList&& params)
      {
        eventCallbackCalled = true;
        eventCallbackParams = std::move(params);
      });

      platform->GetUpdater();
    }

    // Returns a URL or the empty string if there is no such request.
    std::string ProcessPendingUpdateWebRequest()
    {
      auto ii = webRequestTasks->begin();
      while (ii != webRequestTasks->end())
      {
        if (ii->url.find("update.json") != std::string::npos)
        {
          ii->getCallback(webRequestResponse);
          auto url = ii->url;
          webRequestTasks->erase(ii);
          return url;
        }
        ++ii;
      }
      return std::string();
    }

    void ForceUpdateCheck()
    {
      platform->GetUpdater().ForceUpdateCheck([this](const std::string& error)
      {
        updateCallbackCalled = true;
        updateError = error;
      });
      DelayedTimer::ProcessImmediateTimers(timerTasks);
    }
  };
}

TEST_F(UpdateCheckTest, RequestFailure)
{
  webRequestResponse.status = IWebRequest::NS_ERROR_FAILURE;

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = false;

  CreateUpdater();
  ForceUpdateCheck();

  auto requestUrl = ProcessPendingUpdateWebRequest();

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_FALSE(updateError.empty());

  std::string expectedUrl(platform->GetUpdater().GetPref("update_url_release").AsString());
  std::string platform = GetJsEngine().Evaluate("require('info').platform").AsString();
  std::string platformVersion = GetJsEngine().Evaluate("require('info').platformVersion").AsString();

  FindAndReplace(expectedUrl, "%NAME%", appInfo.name);
  FindAndReplace(expectedUrl, "%TYPE%", "1");   // manual update
  expectedUrl += "&addonName=" + appInfo.name +
                 "&addonVersion=" + appInfo.version +
                 "&application=" + appInfo.application +
                 "&applicationVersion=" + appInfo.applicationVersion +
                 "&platform=" + platform +
                 "&platformVersion=" + platformVersion +
                 "&lastVersion=0&downloadCount=0";
  ASSERT_EQ(expectedUrl, requestUrl);
}

TEST_F(UpdateCheckTest, UpdateAvailable)
{
  webRequestResponse.status = IWebRequest::NS_OK;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "{\"1\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  CreateUpdater();
  ForceUpdateCheck();

  auto requestUrl = ProcessPendingUpdateWebRequest();

  ASSERT_TRUE(eventCallbackCalled);
  ASSERT_EQ(1u, eventCallbackParams.size());
  ASSERT_EQ("https://foo.bar/", eventCallbackParams[0].AsString());
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());

  std::string expectedUrl(platform->GetUpdater().GetPref("update_url_devbuild").AsString());
  std::string platform = GetJsEngine().Evaluate("require('info').platform").AsString();
  std::string platformVersion = GetJsEngine().Evaluate("require('info').platformVersion").AsString();

  FindAndReplace(expectedUrl, "%NAME%", appInfo.name);
  FindAndReplace(expectedUrl, "%TYPE%", "1");   // manual update
  expectedUrl += "&addonName=" + appInfo.name +
                 "&addonVersion=" + appInfo.version +
                 "&application=" + appInfo.application +
                 "&applicationVersion=" + appInfo.applicationVersion +
                 "&platform=" + platform +
                 "&platformVersion=" + platformVersion +
                 "&lastVersion=0&downloadCount=0";
  ASSERT_EQ(expectedUrl, requestUrl);
}

TEST_F(UpdateCheckTest, ApplicationUpdateAvailable)
{
  webRequestResponse.status = IWebRequest::NS_OK;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "{\"1/4\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  CreateUpdater();
  ForceUpdateCheck();

  ProcessPendingUpdateWebRequest();

  ASSERT_TRUE(eventCallbackCalled);
  ASSERT_EQ(1u, eventCallbackParams.size());
  ASSERT_EQ("https://foo.bar/", eventCallbackParams[0].AsString());
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongApplication)
{
  webRequestResponse.status = IWebRequest::NS_OK;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "{\"1/3\": {\"version\":\"3.1\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  CreateUpdater();
  ForceUpdateCheck();

  ProcessPendingUpdateWebRequest();

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongVersion)
{
  webRequestResponse.status = IWebRequest::NS_OK;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "{\"1\": {\"version\":\"3\",\"url\":\"https://foo.bar/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  CreateUpdater();
  ForceUpdateCheck();

  ProcessPendingUpdateWebRequest();

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_TRUE(updateError.empty());
}

TEST_F(UpdateCheckTest, WrongURL)
{
  webRequestResponse.status = IWebRequest::NS_OK;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "{\"1\": {\"version\":\"3.1\",\"url\":\"http://insecure/\"}}";

  appInfo.name = "1";
  appInfo.version = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "2";
  appInfo.developmentBuild = true;

  CreateUpdater();
  ForceUpdateCheck();

  ProcessPendingUpdateWebRequest();

  ASSERT_FALSE(eventCallbackCalled);
  ASSERT_TRUE(updateCallbackCalled);
  ASSERT_FALSE(updateError.empty());
}

TEST_F(UpdateCheckTest, SetRemoveUpdateAvailableCallback)
{
  webRequestResponse.status = 0;
  webRequestResponse.responseStatus = 200;
  webRequestResponse.responseText = "\
{\
  \"test\": {\
    \"version\": \"1.0.2\",\
    \"url\": \"https://downloads.adblockplus.org/test-1.0.2.tar.gz?update\"\
  }\
}";

  appInfo.name = "test";
  appInfo.version = "1.0.1";
  CreateUpdater();

  int timesCalled = 0;
  platform->GetUpdater().SetUpdateAvailableCallback([&timesCalled](const std::string&)->void
  {
    ++timesCalled;
  });
  ForceUpdateCheck();

  // ensure that the was the corresponding request
  EXPECT_FALSE(ProcessPendingUpdateWebRequest().empty());

  EXPECT_EQ(1, timesCalled);

  // Updater::SetUpdateAvailableCallback overriddes previously installed on JsEngine
  // handler for "updateAvailable" event.
  EXPECT_FALSE(eventCallbackCalled);

  platform->GetUpdater().RemoveUpdateAvailableCallback();
  ForceUpdateCheck();

  // ensure that the was the corresponding request
  EXPECT_FALSE(ProcessPendingUpdateWebRequest().empty());

  EXPECT_FALSE(eventCallbackCalled);
  EXPECT_EQ(1, timesCalled);

  // previous handler is not restored
  EXPECT_FALSE(eventCallbackCalled);
}
