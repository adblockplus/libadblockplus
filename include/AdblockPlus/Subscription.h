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

#include <AdblockPlus/ISubscriptionImplementation.h>

namespace AdblockPlus
{
  class Subscription
  {
  public:
    explicit Subscription(std::unique_ptr<ISubscriptionImplementation> impl);
    ~Subscription();
    bool IsDisabled() const;
    void SetDisabled(bool value);
    void UpdateFilters();
    bool IsUpdating() const;
    bool IsAA() const;
    std::string GetTitle() const;
    std::string GetUrl() const;
    std::string GetHomepage() const;
    std::string GetAuthor() const;
    std::vector<std::string> GetLanguages() const;
    int GetFilterCount() const;
    std::string GetSynchronizationStatus() const;
    int GetLastDownloadAttemptTime() const;
    int GetLastDownloadSuccessTime() const;
    int GetVersion() const;
    bool operator==(const Subscription& other) const;
    const ISubscriptionImplementation* Implementation() const;
    Subscription& operator=(const Subscription& filter);
    Subscription& operator=(Subscription&& filter);
    Subscription(const Subscription& other);
    Subscription(Subscription&& other);

  private:
    std::unique_ptr<ISubscriptionImplementation> implementation;
  };
}
