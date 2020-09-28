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

#ifndef ADBLOCK_PLUS_FILTER_H
#define ADBLOCK_PLUS_FILTER_H

#include <memory>

#include <AdblockPlus/JsValue.h>

namespace AdblockPlus
{
  /**
   * Adblock Plus Filter object wrapper.
   * @see [original documentation](https://adblockplus.org/jsdoc/adblockpluscore/Filter.html),
   */
  class Filter
  {
  public:
    /**
     * Creates a wrapper for an existing JavaScript filter object.
     * Normally you shouldn't call this directly, but use
     * IFilterEngine::GetFilter() instead.
     * @param object JavaScript filter object.
     * @param engine JavaScript engine to make calls on object.
     */
    Filter(JsValue&& object, JsEngine* jsEngine);

    /**
     * Filter types, see https://adblockplus.org/en/filters.
     */
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

    /**
     * Retrieves the type of this filter.
     * @return Type of this filter.
     */
    Type GetType() const;

    /**
     * Checks whether this filter has been added to the list of custom filters.
     * @return `true` if this filter has been added.
     */
    bool IsListed() const;

    /**
     * Adds this filter to the list of custom filters.
     */
    void AddToList();

    /**
     * Removes this filter from the list of custom filters.
     */
    void RemoveFromList();

    /**
     * Unparsed string representation of the filter.
     * @return unparser filter.
     */
    std::string GetRaw() const;

    bool operator==(const Filter& filter) const;

  private:
    std::string GetStringProperty(const std::string& name) const;

    JsValue jsObject;
    JsEngine* jsEngine;
  };

  /**
   * A smart pointer to a `Filter` instance.
   */
  typedef std::unique_ptr<Filter> FilterPtr;
}

#endif
