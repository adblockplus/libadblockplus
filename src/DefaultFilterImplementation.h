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

#ifndef ADBLOCK_PLUS_DEFAULT_FILTER_H
#define ADBLOCK_PLUS_DEFAULT_FILTER_H

#include <AdblockPlus/IFilterImplementation.h>
#include <AdblockPlus/JsValue.h>

namespace AdblockPlus
{
  class JsEngine;
  class DefaultFilterImplementation : public IFilterImplementation
  {
  public:
    /**
     * Creates a wrapper for an existing JavaScript filter object.
     * Normally you shouldn't call this directly, but use
     * IFilterEngine::GetFilter() instead.
     * @param object JavaScript filter object.
     * @param engine JavaScript engine to make calls on object.
     */
    DefaultFilterImplementation(JsValue&& object, JsEngine* jsEngine);
    IFilterImplementation::Type GetType() const final;
    std::string GetRaw() const final;
    bool operator==(const IFilterImplementation& filter) const final;
    bool IsListed() const final;
    void AddToList() final;
    void RemoveFromList() final;
    std::unique_ptr<IFilterImplementation> Clone() const final;

  private:
    friend class DefaultFilterEngine;
    std::string GetStringProperty(const std::string& name) const;

    JsValue jsObject;
    JsEngine* jsEngine;
  };
}

#endif
