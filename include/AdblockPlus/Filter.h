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

#include <AdblockPlus/IFilterImplementation.h>

namespace AdblockPlus
{
  class Filter
  {
  public:
    using Type = IFilterImplementation::Type;
    Filter();
    ~Filter();
    explicit Filter(std::unique_ptr<IFilterImplementation> impl);
    Type GetType() const;
    std::string GetRaw() const;
    const IFilterImplementation* Implementation() const;
    bool operator==(const Filter& filter) const;
    Filter& operator=(const Filter& filter);
    Filter& operator=(Filter&& filter);
    Filter(const Filter& other);
    Filter(Filter&& other);
    bool IsValid() const;
    /**
     * DEPRECATED. Use IFilterEngine::GetListedFilters() combined with find
     * instead.
     */
    [[deprecated("Use IFilterEngine::GetListedFilters() combined with find"
                 " instead")]]
    bool IsListed() const;
    /**
     * DEPRECATED. Use IFilterEngine::AddFilter() instead.
     */
    [[deprecated("Use IFilterEngine::AddFilter() instead")]]
    void AddToList();
    /**
     * DEPRECATED. Use IFilterEngine::RemoveFilter() instead.
     */
    [[deprecated("Use IFilterEngine::RemoveFilter() instead")]]
    void RemoveFromList();

  private:
    std::unique_ptr<IFilterImplementation> implementation;
  };
}
