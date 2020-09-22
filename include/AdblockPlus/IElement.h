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

#ifndef ADBLOCK_PLUS_IELEMENT_H
#define ADBLOCK_PLUS_IELEMENT_H

#include <string>
#include <vector>

namespace AdblockPlus
{
  /**
   * Simplified representation of DOM Element.
   * Required for IFilterEngine::ComposeFilterSuggestions
   */
  class IElement
  {
  public:
    virtual ~IElement() = default;

    /**
     * Returns local part of the qualified name of the element.
     */
    virtual std::string GetLocalName() const = 0;

    /**
      * Retrieves the value of the named attribute from the current node.
      */
    virtual std::string GetAttribute(const std::string& name) const = 0;

    /**
      * Returns containing document url.
      */
    virtual std::string GetDocumentLocation() const = 0;

    /**
      * Returns collection of child elements.
      */
    virtual std::vector<const IElement*> GetChildren() const = 0;
  };
}

#endif // ADBLOCK_PLUS_IELEMENT_H
