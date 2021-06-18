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

#include <AdblockPlus/ISubscriptionImplementation.h>
#include <AdblockPlus/JsValue.h>

namespace AdblockPlus
{
  class DefaultSubscriptionImplementation : public ISubscriptionImplementation
  {
  public:
    /**
     * Creates a wrapper for an existing JavaScript subscription object.
     * Normally you shouldn't call this directly, but use
     * IFilterEngine::GetSubscription() instead.
     * @param object JavaScript subscription object.
     * @param engine JavaScript engine to make calls on object.
     */
    DefaultSubscriptionImplementation(JsValue&& object, JsEngine* jsEngine);

    bool IsDisabled() const final;
    void SetDisabled(bool value) final;
    void UpdateFilters() final;
    bool IsUpdating() const final;
    bool IsAA() const final;
    std::string GetTitle() const final;
    std::string GetUrl() const final;
    std::string GetHomepage() const final;
    std::string GetAuthor() const final;
    std::vector<std::string> GetLanguages() const final;
    int GetFilterCount() const final;
    std::string GetSynchronizationStatus() const final;
    int GetLastDownloadAttemptTime() const final;
    int GetLastDownloadSuccessTime() const final;
    int GetVersion() const final;
    bool operator==(const ISubscriptionImplementation& value) const final;
    std::unique_ptr<ISubscriptionImplementation> Clone() const final;

  private:
    friend class DefaultFilterEngine;

    std::string GetStringProperty(const std::string& name) const;
    int GetIntProperty(const std::string& name) const;

    JsValue jsObject;
    JsEngine* jsEngine{nullptr};
  };
}
