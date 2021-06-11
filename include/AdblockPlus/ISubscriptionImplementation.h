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

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace AdblockPlus
{
  /**
   * Adblock Plus Subscription object wrapper.
   * @see [original documentation](https://adblockplus.org/jsdoc/adblockpluscore/Subscription.html).
   */
  class ISubscriptionImplementation
  {
  public:
    virtual ~ISubscriptionImplementation() = default;

    /**
     * Returns whether the subscription is disabled.
     * Filters from the disabled subscription are never matched.
     * @return `true` if this subscription is disabled.
     */
    virtual bool IsDisabled() const = 0;

    /**
     * Allows to enable or disable current subscription.
     * @param `value` disabling the subscription if true and enabling if false.
     *        If the previous state was the same then it has no effect.
     */
    virtual void SetDisabled(bool value) = 0;

    /**
     * Updates this subscription, i.e. retrieves the current filters from the
     * subscription URL.
     */
    virtual void UpdateFilters() = 0;

    /**
     * Checks if the subscription is currently being updated.
     * @return `true` if the subscription is currently being updated.
     */
    virtual bool IsUpdating() const = 0;

    /**
     * Indicates whether the subscription is the Acceptable Ads subscription.
     * @return `true` if this subscription is the Acceptable Ads subscription.
     */
    virtual bool IsAA() const = 0;

    virtual std::string GetTitle() const = 0;

    /**
     * Url subscription fetched from.
     * @return url
     */
    virtual std::string GetUrl() const = 0;

    virtual std::string GetHomepage() const = 0;

    virtual std::string GetAuthor() const = 0;

    /**
     * Supported languages list for some recommended subscription.
     * @return languages list, two letter ISO 639-1 code each. Can be empty.
     */
    virtual std::vector<std::string> GetLanguages() const = 0;

    /**
     * Number of filters in this subscription.
     * @returns number of filters
     */
    virtual int GetFilterCount() const = 0;

    /**
     * Status for last attempt to synchronize subscription.
     * @return status, can be empty string or one of following values:
     * - synchronize_ok
     * - synchronize_invalid_data
     * - synchronize_invalid_url
     * - synchronize_connection_error
     */
    virtual std::string GetSynchronizationStatus() const = 0;

    /**
     * Last time the subscription was successfully downloaded or network error recieved.
     * @return unix time or 0 if there were no attempts.
     */
    virtual int GetLastDownloadAttemptTime() const = 0;

    /**
     * Last time the subscription was successfully downloaded.
     * @return unix time or 0 if there were no sucessfull attempts.
     */
    virtual int GetLastDownloadSuccessTime() const = 0;

    virtual bool operator==(const ISubscriptionImplementation& other) const = 0;

    virtual std::unique_ptr<ISubscriptionImplementation> Clone() const = 0;
  };
}
