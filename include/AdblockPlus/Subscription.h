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
   * Adblock Plus Subscription object wrapper.
   * @see [original documentation](https://adblockplus.org/jsdoc/adblockpluscore/Subscription.html).
   */
  class Subscription
  {
  public:
    /**
     * Creates a wrapper for an existing JavaScript subscription object.
     * Normally you shouldn't call this directly, but use
     * IFilterEngine::GetSubscription() instead.
     * @param object JavaScript subscription object.
     * @param engine JavaScript engine to make calls on object.
     */
    Subscription(JsValue&& object, JsEngine* jsEngine);

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

    std::string GetTitle() const;

    /**
      * Url subscription fetched from.
      * @return url
      */
    std::string GetUrl() const;

    std::string GetHomepage() const;

    std::string GetAuthor() const;

    /**
     * Supported languages list for some recommended subscription.
     * @return languages list, two letter ISO 639-1 code each. Can be empty.
     */
    std::vector<std::string> GetLanguages() const;

    /**
      * Number of filters in this subscription.
      * @returns number of filters
      */
    int GetFilterCount() const;

    /**
     * Status for last attempt to synchronize subscription
     * @return status, can be empty string or one of following values:
     * - synchronize_ok
     * - synchronize_invalid_data
     * - synchronize_invalid_url
     * - synchronize_connection_error
     */
    std::string GetSynchronizationStatus() const;

    bool operator==(const Subscription& value) const;

  private:
    std::string GetStringProperty(const std::string& name) const;

    JsValue jsObject;
    JsEngine* jsEngine;
  };
}

#endif
