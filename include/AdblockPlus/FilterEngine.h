/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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

#ifndef ADBLOCK_PLUS_FILTER_ENGINE_H
#define ADBLOCK_PLUS_FILTER_ENGINE_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class FilterEngine;

  class Filter : public JsValue,
                 public std::tr1::enable_shared_from_this<Filter>
  {
  public:
    enum Type {TYPE_BLOCKING, TYPE_EXCEPTION,
               TYPE_ELEMHIDE, TYPE_ELEMHIDE_EXCEPTION,
               TYPE_COMMENT, TYPE_INVALID};

    Type GetType();
    bool IsListed();
    void AddToList();
    void RemoveFromList();
    bool operator==(const Filter& filter) const;

    Filter(JsValuePtr value);
  };

  class Subscription : public JsValue,
                       public std::tr1::enable_shared_from_this<Subscription>
  {
  public:
    bool IsListed();
    void AddToList();
    void RemoveFromList();
    void UpdateFilters();
    bool IsUpdating();
    bool operator==(const Subscription& subscription) const;

    Subscription(JsValuePtr value);
  };

  typedef std::tr1::shared_ptr<Filter> FilterPtr;
  typedef std::tr1::shared_ptr<Subscription> SubscriptionPtr;

  class FilterEngine
  {
  public:
    typedef std::tr1::function<void(const std::string&)> UpdaterCallback;
    typedef std::tr1::function<void(const std::string&, const JsValuePtr)> FilterChangeCallback;

    explicit FilterEngine(JsEnginePtr jsEngine);
    JsEnginePtr GetJsEngine() const { return jsEngine; }
    bool IsFirstRun() const;
    FilterPtr GetFilter(const std::string& text);
    SubscriptionPtr GetSubscription(const std::string& url);
    std::vector<FilterPtr> GetListedFilters() const;
    std::vector<SubscriptionPtr> GetListedSubscriptions() const;
    std::vector<SubscriptionPtr> FetchAvailableSubscriptions() const;
    FilterPtr Matches(const std::string& url,
        const std::string& contentType,
        const std::string& documentUrl) const;
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;
    JsValuePtr GetPref(const std::string& pref) const;
    void SetPref(const std::string& pref, JsValuePtr value);
    void ForceUpdateCheck(UpdaterCallback callback = 0);
    void SetFilterChangeCallback(FilterChangeCallback callback);
    void RemoveFilterChangeCallback();

  private:
    JsEnginePtr jsEngine;
    bool initialized;
    bool firstRun;
    int updateCheckId;

    void InitDone(JsValueList& params);
    void UpdateCheckDone(const std::string& eventName, UpdaterCallback callback, JsValueList& params);
    void FilterChanged(FilterChangeCallback callback, JsValueList& params);
  };
}

#endif
