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

#include "BaseJsTest.h"

using namespace AdblockPlus;

// This define enables NotificationMockWebRequestTest but to run it
// one need to set INITIAL_DELAY to about 2000 msec in notification.js.
//#define NotificationMockWebRequestTest_ENABLED

namespace
{
  typedef std::shared_ptr<FilterEngine> FilterEnginePtr;

  class NotificationTest : public ::testing::Test
  {
  protected:
    FilterEnginePtr filterEngine;
    void SetUp()
    {
      JsEngineCreationParameters jsEngineParams;
      jsEngineParams.fileSystem.reset(new LazyFileSystem());
      jsEngineParams.logSystem.reset(new DefaultLogSystem());
      jsEngineParams.timer.reset(new NoopTimer());
      jsEngineParams.webRequest.reset(new NoopWebRequest());
      auto jsEngine = CreateJsEngine(std::move(jsEngineParams));
      filterEngine = FilterEngine::Create(jsEngine);
    }

    void AddNotification(const std::string& notification)
    {
      filterEngine->GetJsEngine()->Evaluate("(function()"
      "{"
        "require('notification').Notification.addNotification(" + notification + ");"
      "})();");
    }

    std::unique_ptr<Notification> PeekNotification(const std::string& url = std::string())
    {
      std::unique_ptr<Notification> retValue;
      filterEngine->SetShowNotificationCallback(
        [&retValue](Notification&& notification) {
          retValue.reset(new Notification(std::move(notification)));
        });
      filterEngine->ShowNextNotification(url);
      filterEngine->RemoveShowNotificationCallback();
      return retValue;
    }
  };

  class MockWebRequest : public IWebRequest
  {
  public:
    std::string responseText;
    explicit MockWebRequest(const std::string& notification)
      : responseText(notification)
    {
    }
    void GET(const std::string& url, const HeaderList& requestHeaders,
      const GetCallback& getCallback) override
    {
      if (url.find("/notification.json") == std::string::npos)
      {
        return;
      }
      ServerResponse serverResponse;
      serverResponse.status = IWebRequest::NS_OK;
      serverResponse.responseStatus = 200;
      serverResponse.responseText = responseText;
      getCallback(serverResponse);
    }
  };

#ifdef NotificationMockWebRequestTest_ENABLED
  class NotificationMockWebRequestTest : public BaseJsTest
  {
  protected:
    FilterEnginePtr filterEngine;
    bool isNotificationCallbackCalled;
    void SetUp()
    {
      BaseJsTest::SetUp();
      isNotificationCallbackCalled = false;
      jsEngine->SetFileSystem(
        std::shared_ptr<LazyFileSystem>(new LazyFileSystem()));
      const char* responseJsonText = "{"
        "\"notifications\": [{"
          "\"id\": \"some id\","
          "\"type\": \"information\","
          "\"message\": {"
             "\"en-US\": \"message\""
          "},"
          "\"title\": \"Title\""
        "}]"
        "}";
      jsEngine->SetWebRequest(std::shared_ptr<MockWebRequest>(
        new MockWebRequest(responseJsonText)));
      jsEngine->SetLogSystem(LogSystemPtr(new DefaultLogSystem()));
      filterEngine = FilterEngine::Create(jsEngine);
      filterEngine->SetShowNotificationCallback(
        [this](Notification&& notification) {
          isNotificationCallbackCalled = true;
          EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_INFORMATION, notification.GetType());
          EXPECT_EQ("Title", notification.GetTexts().title);
          EXPECT_EQ("message", notification.GetTexts().message);
          notification.MarkAsShown();
        });
    }
  };
#endif
}

TEST_F(NotificationTest, NoNotifications)
{
  EXPECT_FALSE(PeekNotification());
}

#ifdef NotificationMockWebRequestTest_ENABLED
TEST_F(NotificationMockWebRequestTest, SingleNotification)
{
  AdblockPlus::Sleep(5000/*msec*/); // it's a hack
  EXPECT_TRUE(isNotificationCallbackCalled);
}
#endif

TEST_F(NotificationTest, AddNotification)
{
  AddNotification("{"
      "type: 'critical',"
      "title: 'testTitle',"
      "message: 'testMessage',"
    "}");
  auto notification = PeekNotification();
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_CRITICAL, notification->GetType());
  EXPECT_EQ("testTitle", notification->GetTexts().title);
  EXPECT_EQ("testMessage", notification->GetTexts().message);
}

TEST_F(NotificationTest, FilterByUrl)
{
  AddNotification("{ id: 'no-filter', type: 'critical' }");
  AddNotification("{ id: 'www.com', type: 'information',"
    "urlFilters:['||www.com$document']"
  "}");
  AddNotification("{ id: 'www.de', type: 'question',"
    "urlFilters:['||www.de$document']"
  "}");

  auto notification = PeekNotification();
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_CRITICAL, notification->GetType());

  notification = PeekNotification("http://www.de");
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_QUESTION, notification->GetType());

  notification = PeekNotification("http://www.com");
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_INFORMATION, notification->GetType());
}

TEST_F(NotificationTest, MarkAsShown)
{
  AddNotification("{ id: 'id', type: 'question' }");
  EXPECT_TRUE(PeekNotification());
  auto notification = PeekNotification();
  ASSERT_TRUE(notification);
  notification->MarkAsShown();
  EXPECT_FALSE(PeekNotification());
}

TEST_F(NotificationTest, NoLinks)
{
  AddNotification("{ id: 'id'}");
  auto notification = PeekNotification();
  ASSERT_TRUE(notification);
  EXPECT_EQ(0u, notification->GetLinks().size());
}

TEST_F(NotificationTest, Links)
{
  AddNotification("{ id: 'id', links: ['link1', 'link2'] }");
  auto notification = PeekNotification();
  ASSERT_TRUE(notification);
  std::vector<std::string> notificationLinks = notification->GetLinks();
  ASSERT_EQ(2u, notificationLinks.size());
  EXPECT_EQ("link1", notificationLinks[0]);
  EXPECT_EQ("link2", notificationLinks[1]);
}
