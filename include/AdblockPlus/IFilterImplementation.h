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

namespace AdblockPlus
{
  /**
   * Adblock Plus Filter object wrapper.
   * @see [original documentation](https://adblockplus.org/jsdoc/adblockpluscore/Filter.html),
   */
  class IFilterImplementation
  {
  public:
    enum Type
    {
      TYPE_BLOCKING,
      TYPE_EXCEPTION,
      TYPE_ELEMHIDE,
      TYPE_ELEMHIDE_EXCEPTION,
      TYPE_ELEMHIDE_EMULATION,
      TYPE_COMMENT,
      TYPE_INVALID
    };

    virtual ~IFilterImplementation() = default;

    /**
     * Retrieves the type of this filter.
     * @return Type of this filter.
     */
    virtual Type GetType() const = 0;

    /**
     * Unparsed string representation of the filter.
     * @return unparser filter.
     */
    virtual std::string GetRaw() const = 0;

    /**
     * Checks whether this filter has been added to the list of custom filters.
     * @return `true` if this filter has been added.
     *
     * DEPRECATED. Use IFilterEngine::GetListedFilters() combined with find
     * instead.
     */
    virtual bool IsListed() const = 0;

    /**
     * Adds this filter to the list of custom filters.
     *
     * DEPRECATED. Use IFilterEngine::AddFilter() instead.
     */
    virtual void AddToList() = 0;

    /**
     * Removes this filter from the list of custom filters.
     *
     * DEPRECATED. Use IFilterEngine::RemoveFilter() instead.
     */
    virtual void RemoveFromList() = 0;

    virtual bool operator==(const IFilterImplementation& filter) const = 0;

    virtual std::unique_ptr<IFilterImplementation> Clone() const = 0;
  };
}
