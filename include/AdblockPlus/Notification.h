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

#ifndef ADBLOCK_PLUS_NOTIFICATION_H
#define ADBLOCK_PLUS_NOTIFICATION_H

#include <string>
#include <vector>
#include "tr1_memory.h"

namespace AdblockPlus
{
  class FilterEngine;
  /**
   * Possible notification types.
   */
  enum NotificationType
  {
    NOTIFICATION_TYPE_INFORMATION,
    NOTIFICATION_TYPE_QUESTION,
    NOTIFICATION_TYPE_CRITICAL
  };

  /**
   * Wrapper for an Adblock Plus notification object.
   */
  class Notification: public JsValue
  {
    friend class FilterEngine;
  protected:
    static std::tr1::shared_ptr<Notification> JsValueToNotification(const JsValuePtr& jsValue);
    /**
     * Constructor.
     * @param jsValue `JsValuePtr` notification JavaScript object.
     */
    explicit Notification(const JsValuePtr& jsValue);
  public:
    /**
     * Retrieves the type of this notification.
     * @return Type of this notification.
     */
    NotificationType GetType() const;

    /**
     * Retrieves the title of this notification.
     * @return Title of this notification.
     */
    const std::string& GetTitle() const;

    /**
     * Retrieves the message of this notification.
     * @return Message of this notification.
     */
    const std::string& GetMessageString() const;

    /**
     * Marks this notification as shown. It is only relevant for question
     * notifications. Other notifications are marked automatically.
     */
    void MarkAsShown();
  private:
    std::string title;
    std::string message;
    NotificationType type;
  };
  typedef std::tr1::shared_ptr<Notification> NotificationPtr;
}

#endif