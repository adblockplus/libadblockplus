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

#ifndef ADBLOCK_PLUS_SUBSCRIPTION_H
#define ADBLOCK_PLUS_SUBSCRIPTION_H

#include <memory>
#include <AdblockPlus/JsValue.h>

namespace AdblockPlus
{
  /**
   * Wrapper for a subscription object.
   * There are no accessors for most
   * [subscription properties](https://adblockplus.org/jsdoc/adblockpluscore/Subscription.html),
   * use `GetProperty()` to retrieve them by name.
   */
  class Subscription : public JsValue
  {
  public:
    Subscription(const Subscription& src);
    Subscription(Subscription&& src);
    Subscription& operator=(const Subscription& src);
    Subscription& operator=(Subscription&& src);
    /**
     * Creates a wrapper for an existing JavaScript subscription object.
     * Normally you shouldn't call this directly, but use
     * FilterEngine::GetSubscription() instead.
     * @param value JavaScript subscription object.
     */
    Subscription(JsValue&& value);

    /**
     * Checks if the subscription is disabled.
     * @return `true` if this subscription is disabled.
     */
    bool IsDisabled() const;

    /**
     * Allows to enable or disable current subscription.
     * @param `value` disabling the subscription if true and enabling if false.
     *        If the previous state was the same then it has no effect.
     */
    void SetDisabled(bool value);

    /**
     * Checks if this subscription has been added to the list of subscriptions.
     * @return `true` if this subscription has been added.
     */
    bool IsListed() const;

    /**
     * Adds this subscription to the list of subscriptions.
     */
    void AddToList();

    /**
     * Removes this subscription from the list of subscriptions.
     */
    void RemoveFromList();

    /**
     * Updates this subscription, i.e.\ retrieves the current filters from the
     * subscription URL.
     */
    void UpdateFilters();

    /**
     * Checks if the subscription is currently being updated.
     * @return `true` if the subscription is currently being updated.
     */
    bool IsUpdating() const;

    /**
     * Indicates whether the subscription is the Acceptable Ads subscription.
     * @return `true` if this subscription is the Acceptable Ads subscription.
     */
    bool IsAA() const;

    bool operator==(const Subscription& subscription) const;
  };
}

#endif
