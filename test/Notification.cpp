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

#include "BaseJsTest.h"

using namespace AdblockPlus;

// This define enables NotificationMockWebRequestTest but to run it
// one need to set INITIAL_DELAY to about 2000 msec in notification.js.
//#define NotificationMockWebRequestTest_ENABLED

namespace
{
  typedef std::shared_ptr<FilterEngine> FilterEnginePtr;

  class NotificationTest : public BaseJsTest
  {
  protected:
    FilterEnginePtr filterEngine;
    void SetUp()
    {
      BaseJsTest::SetUp();
      jsEngine->SetFileSystem(FileSystemPtr(new LazyFileSystem()));
      jsEngine->SetWebRequest(WebRequestPtr(new LazyWebRequest()));
      jsEngine->SetLogSystem(LogSystemPtr(new DefaultLogSystem()));
      filterEngine = FilterEngine::Create(jsEngine);
    }

    void AddNotification(const std::string& notification)
    {
      jsEngine->Evaluate("(function()"
      "{"
        "require('notification').Notification.addNotification(" + notification + ");"
      "})();");
    }

    NotificationPtr PeekNotification(const std::string& url = std::string())
    {
      NotificationPtr retValue;
      filterEngine->SetShowNotificationCallback(std::bind(
        &NotificationTest::NotificationAvailableCallback,
        std::placeholders::_1, std::ref(retValue)));
      filterEngine->ShowNextNotification(url);
      filterEngine->RemoveShowNotificationCallback();
      return retValue;
    }

    static void NotificationAvailableCallback(const NotificationPtr& src, NotificationPtr& dst)
    {
      EXPECT_TRUE(src);
      dst = src;
    }
  };

  class MockWebRequest : public WebRequest
  {
  public:
    std::string responseText;
    explicit MockWebRequest(const std::string& notification)
      : responseText(notification)
    {
    }
    ServerResponse GET(const std::string& url,
      const HeaderList& requestHeaders) const
    {
      if (url.find("/notification.json") == std::string::npos)
      {
        return ServerResponse();
      }
      ServerResponse serverResponse;
      serverResponse.status = NS_OK;
      serverResponse.responseStatus = 200;
      serverResponse.responseText = responseText;
      return serverResponse;
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
      filterEngine.reset(new FilterEngine(jsEngine));
      filterEngine->SetShowNotificationCallback(
        std::bind(&NotificationMockWebRequestTest::OnNotification,
        this, std::placeholders::_1));
    }

    void OnNotification(const NotificationPtr& notification)
    {
      isNotificationCallbackCalled = true;
      ASSERT_TRUE(notification);
      EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_INFORMATION, notification->GetType());
      EXPECT_EQ("Title", notification->GetTexts().title);
      EXPECT_EQ("message", notification->GetTexts().message);
      notification->MarkAsShown();
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
  NotificationPtr notification = PeekNotification();
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

  NotificationPtr notification = PeekNotification();
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
  NotificationPtr notification = PeekNotification();
  ASSERT_TRUE(notification);
  notification->MarkAsShown();
  EXPECT_FALSE(PeekNotification());
}

TEST_F(NotificationTest, NoLinks)
{
  AddNotification("{ id: 'id'}");
  NotificationPtr notification = PeekNotification();
  ASSERT_TRUE(notification);
  EXPECT_EQ(0, notification->GetLinks().size());
}

TEST_F(NotificationTest, Links)
{
  AddNotification("{ id: 'id', links: ['link1', 'link2'] }");
  NotificationPtr notification = PeekNotification();
  ASSERT_TRUE(notification);
  std::vector<std::string> notificationLinks = notification->GetLinks();
  ASSERT_EQ(2, notificationLinks.size());
  EXPECT_EQ("link1", notificationLinks[0]);
  EXPECT_EQ("link2", notificationLinks[1]);
}
