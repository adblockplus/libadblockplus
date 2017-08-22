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

#ifndef ADBLOCK_PLUS_NOTIFICATION_H
#define ADBLOCK_PLUS_NOTIFICATION_H

#include <string>
#include <vector>
#include <memory>

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
    NOTIFICATION_TYPE_RELENTLESS,
    NOTIFICATION_TYPE_CRITICAL
  };

  /**
   * Contains notification title and message. It's returned by
   * `Notification::GetTexts`.
   */
  struct NotificationTexts
  {
    std::string title;
    std::string message;
  };

  /**
   * Wrapper for an Adblock Plus notification object.
   */
  class Notification: public JsValue
  {
    friend class FilterEngine;
  protected:
    /**
     * Constructor.
     * @param jsValue `JsValue&&` notification JavaScript object.
     */
    explicit Notification(JsValue&& jsValue);
  public:
    /**
     * Copy constructor
     */
    Notification(const Notification& src);

    /**
     * Move constructor
     */
    Notification(Notification&& src);

    /**
     * Assignment operator
     */
    Notification& operator=(const Notification& src);

    /**
     * Move assignment operator
     */
    Notification& operator=(Notification&& src);

    /**
     * Retrieves the type of this notification.
     * @return Type of this notification.
     */
    NotificationType GetType() const;

    /**
     * Retrieves the title and message of this notification.
     * @return Translated texts.
     */
    NotificationTexts GetTexts() const;

    /**
     * Retrieves the URLs which should be mapped to the links in the message.
     * @return List of links.
     */
    std::vector<std::string> GetLinks() const;

    /**
     * Marks this notification as shown. It is only relevant for question
     * notifications. Other notifications are marked automatically.
     */
    void MarkAsShown();
  private:
  };
}

#endif
