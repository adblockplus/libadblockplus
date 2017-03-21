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

#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/Notification.h>
#include <algorithm>

using namespace AdblockPlus;

namespace
{
  typedef std::pair<NotificationType, std::string> NotificationTypeString;
  typedef std::vector<NotificationTypeString> NotificationTypes;
  NotificationTypes InitNotificationTypes()
  {
    NotificationTypes retValue;
    retValue.push_back(std::make_pair(NotificationType::NOTIFICATION_TYPE_QUESTION, "question"));
    retValue.push_back(std::make_pair(NotificationType::NOTIFICATION_TYPE_CRITICAL, "critical"));
    retValue.push_back(std::make_pair(NotificationType::NOTIFICATION_TYPE_INFORMATION, "information"));
    return retValue;
  }

  const NotificationTypes notificationTypes = InitNotificationTypes();

  NotificationType StringToNotificationType(const std::string& value)
  {
    struct IsSecondEqualToPredicate
    {
      std::string value;
      bool operator()(const NotificationTypeString& pair) const
      {
        return value == pair.second;
      }
    } findBySecond = {value};
    NotificationTypes::const_iterator notificationTypeIterator = std::find_if(
      notificationTypes.begin(), notificationTypes.end(), findBySecond);
    if (notificationTypeIterator == notificationTypes.end())
    {
      return NotificationType::NOTIFICATION_TYPE_INFORMATION;
    }
    return notificationTypeIterator->first;
  }
}

Notification::Notification(JsValue&& jsValue)
  : JsValue(std::move(jsValue))
{
}

NotificationType Notification::GetType() const
{
  return StringToNotificationType(GetProperty("type")->AsString());
}

NotificationTexts Notification::GetTexts() const
{
  JsValuePtr jsTexts = jsEngine->Evaluate("API.getNotificationTexts")->Call(*this);
  NotificationTexts notificationTexts;
  JsValuePtr jsTitle = jsTexts->GetProperty("title");
  if (jsTitle->IsString())
  {
    notificationTexts.title = jsTitle->AsString();
  }
  JsValuePtr jsMessage = jsTexts->GetProperty("message");
  if (jsMessage->IsString())
  {
    notificationTexts.message = jsMessage->AsString();
  }
  return notificationTexts;
}

std::vector<std::string> Notification::GetLinks() const
{
  std::vector<std::string> retValue;
  JsValuePtr jsLinks = GetProperty("links");
  if (!jsLinks->IsArray())
  {
    return retValue;
  }
  JsValueList urlLinksList = jsLinks->AsList();
  for (JsValueList::const_iterator linkIterator = urlLinksList.begin();
    linkIterator != urlLinksList.end(); ++linkIterator)
  {
    retValue.push_back((*linkIterator)->AsString());
  }
  return retValue;
}

void Notification::MarkAsShown()
{
  jsEngine->Evaluate("API.markNotificationAsShown")->Call(*GetProperty("id"));
}