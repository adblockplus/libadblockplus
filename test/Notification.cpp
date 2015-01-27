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

#include "BaseJsTest.h"

using namespace AdblockPlus;

// This define enables NotificationMockWebRequestTest but to run it
// one need to set INITIAL_DELAY to about 2000 msec in notification.js.
//#define NotificationMockWebRequestTest_ENABLED

namespace
{
  typedef std::tr1::shared_ptr<FilterEngine> FilterEnginePtr;

  class NotificationTest : public BaseJsTest
  {
  protected:
    FilterEnginePtr filterEngine;
    void SetUp() override
    {
      BaseJsTest::SetUp();
      jsEngine->SetFileSystem(FileSystemPtr(new LazyFileSystem()));
      jsEngine->SetWebRequest(WebRequestPtr(new LazyWebRequest()));
      jsEngine->SetLogSystem(LogSystemPtr(new DefaultLogSystem()));
      filterEngine = std::tr1::make_shared<FilterEngine>(jsEngine);
    }

    void AddNotification(const std::string& notification)
    {
      jsEngine->Evaluate("(function()"
      "{"
        "require('notification').Notification.addNotification(" + notification + ");"
      "})();");
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
    void SetUp() override
    {
      BaseJsTest::SetUp();
      jsEngine->SetFileSystem(std::tr1::make_shared<LazyFileSystem>());
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
      jsEngine->SetWebRequest(std::tr1::make_shared<MockWebRequest>(responseJsonText));
      jsEngine->SetLogSystem(LogSystemPtr(new DefaultLogSystem()));
      filterEngine = std::tr1::make_shared<FilterEngine>(jsEngine);
    }
  };
#endif
}

TEST_F(NotificationTest, NoNotifications)
{
  NotificationPtr notification = filterEngine->GetNextNotificationToShow();
  EXPECT_EQ(NULL, notification.get());
}

#ifdef NotificationMockWebRequestTest_ENABLED
TEST_F(NotificationMockWebRequestTest, SingleNotification)
{
  AdblockPlus::Sleep(5000/*msec*/); // it's a hack
  NotificationPtr notification = filterEngine->GetNextNotificationToShow();
  // try another one immediately to avoid queuing of the next notification by
  // the timer.
  EXPECT_EQ(NULL, filterEngine->GetNextNotificationToShow().get());
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_INFORMATION, notification->GetType());
  EXPECT_EQ("Title", notification->GetTitle());
  EXPECT_EQ("message", notification->GetMessageString());
}
#endif

TEST_F(NotificationTest, AddNotification)
{
  AddNotification("{"
      "type: 'critical',"
      "title: 'testTitle',"
      "message: 'testMessage',"
    "}");
  NotificationPtr notification = filterEngine->GetNextNotificationToShow();
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_CRITICAL, notification->GetType());
  EXPECT_EQ("testTitle", notification->GetTitle());
  EXPECT_EQ("testMessage", notification->GetMessageString());
}

TEST_F(NotificationTest, FilterByUrl)
{
  AddNotification("{ id: 'no-filter', type: 'critical' }");
  AddNotification("{ id: 'www.com', type: 'information',"
    "urlFilters:['http://www.com']"
  "}");
  AddNotification("{ id: 'www.de', type: 'question',"
    "urlFilters:['http://www.de']"
  "}");

  NotificationPtr notification = filterEngine->GetNextNotificationToShow();
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_CRITICAL, notification->GetType());

  notification = filterEngine->GetNextNotificationToShow("http://www.de");
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_QUESTION, notification->GetType());

  notification = filterEngine->GetNextNotificationToShow("http://www.com");
  ASSERT_TRUE(notification);
  EXPECT_EQ(NotificationType::NOTIFICATION_TYPE_INFORMATION, notification->GetType());
}

TEST_F(NotificationTest, MarkAsShown)
{
  AddNotification("{ id: 'id', type: 'question' }");
  NotificationPtr notification = filterEngine->GetNextNotificationToShow();
  EXPECT_TRUE(notification);
  notification = filterEngine->GetNextNotificationToShow();
  ASSERT_TRUE(notification);
  notification->MarkAsShown();
  EXPECT_EQ(NULL, filterEngine->GetNextNotificationToShow().get());
}