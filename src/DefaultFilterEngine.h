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

#include <AdblockPlus/IFilterEngine.h>

namespace AdblockPlus
{
  class DefaultFilterEngine : public IFilterEngine
  {
  public:
    explicit DefaultFilterEngine(JsEngine& jsEngine);
    ~DefaultFilterEngine();

    Filter GetFilter(const std::string& text) const final;

    Subscription GetSubscription(const std::string& url) const final;
    std::vector<Subscription> GetSubscriptionsFromFilter(const Filter& filter) const final;

    std::vector<Filter> GetListedFilters() const final;

    std::vector<Subscription> GetListedSubscriptions() const final;

    std::vector<Subscription> FetchAvailableSubscriptions() const final;

    void SetAAEnabled(bool enabled) final;

    bool IsAAEnabled() const final;

    std::string GetAAUrl() const final;

    Filter Matches(const std::string& url,
                   ContentTypeMask contentTypeMask,
                   const std::string& documentUrl,
                   const std::string& siteKey = "",
                   bool specificOnly = false) const final;

    bool IsContentAllowlisted(const std::string& url,
                              ContentTypeMask contentTypeMask,
                              const std::vector<std::string>& documentUrls,
                              const std::string& sitekey = "") const final;

    std::string GetElementHidingStyleSheet(const std::string& domain,
                                           bool specificOnly = false) const final;

    std::vector<EmulationSelector>
    GetElementHidingEmulationSelectors(const std::string& domain) const final;

    std::string GetHostFromURL(const std::string& url) const final;

    void SetFilterChangeCallback(const FilterChangeCallback& callback) final;
    void AddEventObserver(EventObserver* observer) final;
    void RemoveEventObserver(EventObserver* observer) final;
    void RemoveFilterChangeCallback() final;

    void SetAllowedConnectionType(const std::string* value) final;

    std::unique_ptr<std::string> GetAllowedConnectionType() const final;

    bool VerifySignature(const std::string& key,
                         const std::string& signature,
                         const std::string& uri,
                         const std::string& host,
                         const std::string& userAgent) const final;

    std::vector<std::string> ComposeFilterSuggestions(const IElement* element) const final;

    void AddSubscription(const Subscription& subscripton) final;
    void RemoveSubscription(const Subscription& subscription) final;
    void AddFilter(const Filter& filter) final;
    void RemoveFilter(const Filter& filter) final;
    void StartSynchronization() final;
    void StopSynchronization() final;

    void StartObservingEvents();

  private:
    class Observer : public EventObserver
    {
    public:
      explicit Observer(JsEngine& engine) : jsEngine(engine)
      {
      }

      ~Observer() override = default;

      void OnFilterEvent(FilterEvent, const Filter&) override;

    private:
      JsEngine& jsEngine;
    };

    JsEngine& jsEngine;

    JsValue GetPref(const std::string& pref) const;
    void SetPref(const std::string& pref, const JsValue& value);

    Filter CheckFilterMatch(const std::string& url,
                            ContentTypeMask contentTypeMask,
                            const std::string& documentUrl,
                            const std::string& siteKey,
                            bool specificOnly) const;

    void OnSubscriptionOrFilterChanged(JsValueList&& params) const;
    Filter GetAllowlistingFilter(const std::string& url,
                                 ContentTypeMask contentTypeMask,
                                 const std::vector<std::string>& documentUrls,
                                 const std::string& sitekey) const;
    static bool Transform(const std::string& str, FilterEvent* event);
    static bool Transform(const std::string& str, SubscriptionEvent* event);

    mutable std::mutex callbacksMutex;
    FilterChangeCallback legacyCallback;
    Observer observer{jsEngine};
    std::vector<IFilterEngine::EventObserver*> observers;
  };
}
