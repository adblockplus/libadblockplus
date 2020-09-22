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

#ifndef ADBLOCK_PLUS_FILTER_ENGINE_IMPL_H
#define ADBLOCK_PLUS_FILTER_ENGINE_IMPL_H

#include <AdblockPlus/IFilterEngine.h>

namespace AdblockPlus
{
  class DefaultFilterEngine : public IFilterEngine
  {
  public:
    explicit DefaultFilterEngine(JsEngine& jsEngine);

    JsEngine& GetJsEngine() const final { return jsEngine; }

    bool IsFirstRun() const final;

    Filter GetFilter(const std::string& text) const final;

    Subscription GetSubscription(const std::string& url) const final;

    std::vector<Filter> GetListedFilters() const final;

    std::vector<Subscription> GetListedSubscriptions() const final;

    std::vector<Subscription> FetchAvailableSubscriptions() const final;

    void SetAAEnabled(bool enabled) final;

    bool IsAAEnabled() const final;

    std::string GetAAUrl() const final;

    FilterPtr Matches(const std::string& url,
        ContentTypeMask contentTypeMask,
        const std::string& documentUrl,
        const std::string& siteKey = "",
        bool specificOnly = false) const final;

    FilterPtr Matches(const std::string& url,
        ContentTypeMask contentTypeMask,
        const std::vector<std::string>& documentUrls,
        const std::string& siteKey = "",
        bool specificOnly = false) const final;

    bool IsGenericblockWhitelisted(const std::string& url,
                                   const std::vector<std::string>& documentUrls,
                                   const std::string& sitekey = "") const final;

    bool IsDocumentWhitelisted(const std::string& url,
        const std::vector<std::string>& documentUrls,
        const std::string& sitekey = "") const final;

    bool IsElemhideWhitelisted(const std::string& url,
        const std::vector<std::string>& documentUrls,
        const std::string& sitekey = "") const final;

    std::string GetElementHidingStyleSheet(const std::string& domain, bool specificOnly = false) const final;

    std::vector<EmulationSelector> GetElementHidingEmulationSelectors(const std::string& domain) const final;

    JsValue GetPref(const std::string& pref) const final;

    void SetPref(const std::string& pref, const JsValue& value) final;

    std::string GetHostFromURL(const std::string& url) const final;

    void SetFilterChangeCallback(const FilterChangeCallback& callback) final;

    void RemoveFilterChangeCallback() final;

    void SetAllowedConnectionType(const std::string* value) final;

    std::unique_ptr<std::string> GetAllowedConnectionType() const final;

    int CompareVersions(const std::string& v1, const std::string& v2) const final;

    bool VerifySignature(const std::string& key, const std::string& signature, const std::string& uri,
                         const std::string& host, const std::string& userAgent) const final;

    std::vector<std::string> ComposeFilterSuggestions(const IElement* element) const final;

    void SetIsFirstRun(bool isFirstRun) { firstRun = isFirstRun; }

  private:
    JsEngine& jsEngine;
    bool firstRun;

    FilterPtr CheckFilterMatch(const std::string& url,
                               ContentTypeMask contentTypeMask,
                               const std::string& documentUrl,
                               const std::string& siteKey,
                               bool specificOnly) const;
    void FilterChanged(const FilterChangeCallback& callback, JsValueList&& params) const;
    FilterPtr GetWhitelistingFilter(const std::string& url,
      ContentTypeMask contentTypeMask, const std::string& documentUrl,
      const std::string& sitekey) const;
    FilterPtr GetWhitelistingFilter(const std::string& url,
      ContentTypeMask contentTypeMask,
      const std::vector<std::string>& documentUrls,
      const std::string& sitekey) const;
  };
}

#endif
