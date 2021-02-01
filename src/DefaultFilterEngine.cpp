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

#include "DefaultFilterEngine.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>

#include "DefaultFilterImplementation.h"
#include "DefaultSubscriptionImplementation.h"
#include "ElementUtils.h"
#include "JsContext.h"

using namespace AdblockPlus;

DefaultFilterEngine::DefaultFilterEngine(JsEngine& jsEngine) : jsEngine(jsEngine)
{
  jsEngine.SetEventCallback("filterChange", [this](JsValueList&& params) {
    this->OnSubscriptionOrFilterChanged(move(params));
  });
}

DefaultFilterEngine::~DefaultFilterEngine()
{
  jsEngine.RemoveEventCallback("filterChange");
}

void DefaultFilterEngine::SetEnabled(bool enabled)
{
  JsValue func = jsEngine.Evaluate("API.setFilterEngineEnabled");
  func.Call(jsEngine.NewValue(enabled));
}

bool DefaultFilterEngine::IsEnabled() const
{
  return jsEngine.Evaluate("API.isFilterEngineEnabled()").AsBool();
}

Filter DefaultFilterEngine::GetFilter(const std::string& text) const
{
  JsValue func = jsEngine.Evaluate("API.getFilterFromText");
  return Filter(
      std::make_unique<DefaultFilterImplementation>(func.Call(jsEngine.NewValue(text)), &jsEngine));
}

Subscription DefaultFilterEngine::GetSubscription(const std::string& url) const
{
  JsValue func = jsEngine.Evaluate("API.getSubscriptionFromUrl");
  return Subscription(std::make_unique<DefaultSubscriptionImplementation>(
      func.Call(jsEngine.NewValue(url)), &jsEngine));
}

std::vector<Filter> DefaultFilterEngine::GetListedFilters() const
{
  JsValue func = jsEngine.Evaluate("API.getListedFilters");
  JsValueList values = func.Call().AsList();
  std::vector<Filter> result;
  for (auto& value : values)
    result.emplace_back(
        Filter(std::make_unique<DefaultFilterImplementation>(std::move(value), &jsEngine)));
  return result;
}

std::vector<Subscription> DefaultFilterEngine::GetListedSubscriptions() const
{
  JsValue func = jsEngine.Evaluate("API.getListedSubscriptions");
  JsValueList values = func.Call().AsList();
  std::vector<Subscription> result;
  for (auto& value : values)
    result.emplace_back(Subscription(
        std::make_unique<DefaultSubscriptionImplementation>(std::move(value), &jsEngine)));
  return result;
}

std::vector<Subscription> DefaultFilterEngine::FetchAvailableSubscriptions() const
{
  JsValue func = jsEngine.Evaluate("API.getRecommendedSubscriptions");
  JsValueList values = func.Call().AsList();
  std::vector<Subscription> result;
  for (auto& value : values)
    result.emplace_back(Subscription(
        std::make_unique<DefaultSubscriptionImplementation>(std::move(value), &jsEngine)));
  return result;
}

void DefaultFilterEngine::SetAAEnabled(bool enabled)
{
  jsEngine.Evaluate("API.setAASubscriptionEnabled").Call(jsEngine.NewValue(enabled));
}

bool DefaultFilterEngine::IsAAEnabled() const
{
  return jsEngine.Evaluate("API.isAASubscriptionEnabled()").AsBool();
}

std::string DefaultFilterEngine::GetAAUrl() const
{
  return GetPref("subscriptions_exceptionsurl").AsString();
}

Filter DefaultFilterEngine::Matches(const std::string& url,
                                    ContentTypeMask contentTypeMask,
                                    const std::string& documentUrl,
                                    const std::string& siteKey,
                                    bool specificOnly) const
{
  assert(IsEnabled());
  if (documentUrl.empty())
  {
    // We must be at the top of the frame hierarchy.
    return CheckFilterMatch(url, contentTypeMask, "", siteKey, specificOnly);
  }
  return CheckFilterMatch(url, contentTypeMask, documentUrl, siteKey, specificOnly);
}

bool DefaultFilterEngine::IsContentAllowlisted(const std::string& url,
                                               ContentTypeMask contentTypeMask,
                                               const std::vector<std::string>& documentUrls,
                                               const std::string& sitekey) const
{
  return GetAllowlistingFilter(url, contentTypeMask, documentUrls, sitekey).IsValid();
}

// |documentUrl| gets converted to a hostname (domain) within "API.checkFilterMatch".
Filter DefaultFilterEngine::CheckFilterMatch(const std::string& url,
                                             ContentTypeMask contentTypeMask,
                                             const std::string& documentUrl,
                                             const std::string& siteKey,
                                             bool specificOnly) const
{
  if (url.empty())
    return Filter();
  JsValue func = jsEngine.Evaluate("API.checkFilterMatch");
  JsValueList params;
  params.push_back(jsEngine.NewValue(url));
  params.push_back(jsEngine.NewValue(contentTypeMask));
  params.push_back(jsEngine.NewValue(documentUrl));
  params.push_back(jsEngine.NewValue(siteKey));
  params.push_back(jsEngine.NewValue(specificOnly));
  JsValue result = func.Call(params);
  if (!result.IsNull())
    return Filter(std::make_unique<DefaultFilterImplementation>(std::move(result), &jsEngine));
  else
    return Filter();
}

std::string DefaultFilterEngine::GetElementHidingStyleSheet(const std::string& domain,
                                                            bool specificOnly) const
{
  assert(IsEnabled());
  JsValueList params;
  params.push_back(jsEngine.NewValue(domain));
  params.push_back(jsEngine.NewValue(specificOnly));
  JsValue func = jsEngine.Evaluate("API.getElementHidingStyleSheet");
  return func.Call(params).AsString();
}

std::vector<IFilterEngine::EmulationSelector>
DefaultFilterEngine::GetElementHidingEmulationSelectors(const std::string& domain) const
{
  assert(IsEnabled());
  JsValue func = jsEngine.Evaluate("API.getElementHidingEmulationSelectors");
  JsValueList result = func.Call(jsEngine.NewValue(domain)).AsList();
  std::vector<IFilterEngine::EmulationSelector> selectors;
  selectors.reserve(result.size());
  for (const auto& r : result)
    selectors.push_back({r.GetProperty("selector").AsString(), r.GetProperty("text").AsString()});
  return selectors;
}

JsValue DefaultFilterEngine::GetPref(const std::string& pref) const
{
  JsValue func = jsEngine.Evaluate("API.getPref");
  return func.Call(jsEngine.NewValue(pref));
}

void DefaultFilterEngine::SetPref(const std::string& pref, const JsValue& value)
{
  JsValue func = jsEngine.Evaluate("API.setPref");
  JsValueList params;
  params.push_back(jsEngine.NewValue(pref));
  params.push_back(value);
  func.Call(params);
}

std::string DefaultFilterEngine::GetHostFromURL(const std::string& url) const
{
  JsValue func = jsEngine.Evaluate("API.getHostFromUrl");
  return func.Call(jsEngine.NewValue(url)).AsString();
}

void DefaultFilterEngine::SetFilterChangeCallback(const FilterChangeCallback& callback)
{
  std::unique_lock<std::mutex> lock(callbacksMutex);
  legacyCallback = callback;
}

void DefaultFilterEngine::AddEventObserver(EventObserver* observer)
{
  std::unique_lock<std::mutex> lock(callbacksMutex);
  assert(std::find(observers.begin(), observers.end(), observer) == observers.end());
  observers.push_back(observer);
}

void DefaultFilterEngine::RemoveFilterChangeCallback()
{
  std::unique_lock<std::mutex> lock(callbacksMutex);
  legacyCallback = nullptr;
}

void DefaultFilterEngine::RemoveEventObserver(EventObserver* observer)
{
  std::unique_lock<std::mutex> lock(callbacksMutex);
  auto registered = std::find(observers.begin(), observers.end(), observer);
  assert(registered != observers.end());
  observers.erase(registered);
}

void DefaultFilterEngine::SetAllowedConnectionType(const std::string* value)
{
  SetPref("allowed_connection_type", value ? jsEngine.NewValue(*value) : jsEngine.NewValue(""));
}

// static
bool DefaultFilterEngine::Transform(const std::string& eventStr, FilterEvent* event)
{
  if (eventStr == "load")
  {
    *event = FilterEvent::FILTERS_LOAD;
    return true;
  }

  if (eventStr == "save")
  {
    *event = FilterEvent::FILTERS_SAVE;
    return true;
  }

  if (eventStr == "filter.added")
  {
    *event = FilterEvent::FILTER_ADDED;
    return true;
  }

  if (eventStr == "filter.removed")
  {
    *event = FilterEvent::FILTER_REMOVED;
    return true;
  }

  if (eventStr == "filter.moved")
  {
    *event = FilterEvent::FILTER_MOVED;
    return true;
  }

  if (eventStr == "filter.disabled")
  {
    *event = FilterEvent::FILTER_DISABLED;
    return true;
  }

  if (eventStr == "filter.hitCount")
  {
    *event = FilterEvent::FILTER_HITCOUNT;
    return true;
  }

  if (eventStr == "filter.lastHit")
  {
    *event = FilterEvent::FILTER_LASTHIT;
    return true;
  }

  return false;
}

// static
bool DefaultFilterEngine::Transform(const std::string& eventStr, SubscriptionEvent* event)
{
  if (eventStr == "subscription.added")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_ADDED;
    return true;
  }

  if (eventStr == "subscription.removed")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_REMOVED;
    return true;
  }

  if (eventStr == "subscription.disabled")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_DISABLED;
    return true;
  }

  if (eventStr == "subscription.downloading")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_DOWNLOADING;
    return true;
  }

  if (eventStr == "subscription.downloadStatus")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_DOWNLOADSTATUS;
    return true;
  }

  if (eventStr == "subscription.errors")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_ERRORS;
    return true;
  }

  if (eventStr == "subscription.fixedTitle")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_FIXEDTITLE;
    return true;
  }

  if (eventStr == "subscription.title")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_TITLE;
    return true;
  }

  if (eventStr == "subscription.homepage")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_HOMEPAGE;
    return true;
  }

  if (eventStr == "subscription.lastCheck")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_LASTCHECK;
    return true;
  }

  if (eventStr == "subscription.lastDownload")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_LASTDOWNLOAD;
    return true;
  }

  if (eventStr == "subscription.updated")
  {
    *event = SubscriptionEvent::SUBSCRIPTION_UPDATED;
    return true;
  }

  return false;
}

std::unique_ptr<std::string> DefaultFilterEngine::GetAllowedConnectionType() const
{
  auto prefValue = GetPref("allowed_connection_type");
  if (prefValue.AsString().empty())
    return nullptr;
  return std::unique_ptr<std::string>(new std::string(prefValue.AsString()));
}

void DefaultFilterEngine::OnSubscriptionOrFilterChanged(JsValueList&& params) const
{
  std::string action(params.size() >= 1 && !params[0].IsNull() ? params[0].AsString() : "");
  JsValue item(params.size() >= 2 ? params[1] : jsEngine.NewValue(false));

  std::unique_lock<std::mutex> lock(callbacksMutex);
  if (legacyCallback)
  {
    JsValue copy = item;
    legacyCallback(action, std::move(copy));
  }

  FilterEvent filterEvent;
  SubscriptionEvent subscriptionEvent;
  if (Transform(action, &filterEvent))
  {
    Filter filter(item.IsObject()
                      ? std::make_unique<DefaultFilterImplementation>(std::move(item), &jsEngine)
                      : nullptr);
    for (auto* observer : observers)
    {
      observer->OnFilterEvent(filterEvent, filter);
    }
  }
  else if (Transform(action, &subscriptionEvent))
  {
    Subscription subscription(item.IsObject() ? std::make_unique<DefaultSubscriptionImplementation>(
                                                    std::move(item), &jsEngine)
                                              : nullptr);

    for (auto* observer : observers)
    {
      observer->OnSubscriptionEvent(subscriptionEvent, subscription);
    }
  }
}

bool DefaultFilterEngine::VerifySignature(const std::string& key,
                                          const std::string& signature,
                                          const std::string& uri,
                                          const std::string& host,
                                          const std::string& userAgent) const
{
  JsValueList params;
  params.push_back(jsEngine.NewValue(key));
  params.push_back(jsEngine.NewValue(signature));
  params.push_back(jsEngine.NewValue(uri));
  params.push_back(jsEngine.NewValue(host));
  params.push_back(jsEngine.NewValue(userAgent));
  JsValue func = jsEngine.Evaluate("API.verifySignature");
  return func.Call(params).AsBool();
}

std::vector<std::string>
DefaultFilterEngine::ComposeFilterSuggestions(const IElement* element) const
{
  JsValueList params;

  params.push_back(jsEngine.NewValue(element->GetDocumentLocation()));
  params.push_back(jsEngine.NewValue(element->GetLocalName()));
  params.push_back(jsEngine.NewValue(element->GetAttribute("id")));
  params.push_back(jsEngine.NewValue(element->GetAttribute("src")));
  params.push_back(jsEngine.NewValue(element->GetAttribute("style")));
  params.push_back(jsEngine.NewValue(element->GetAttribute("class")));
  params.push_back(jsEngine.NewArray(Utils::GetAssociatedUrls(element)));

  JsValue func = jsEngine.Evaluate("API.composeFilterSuggestions");
  JsValueList suggestions = func.Call(params).AsList();
  std::vector<std::string> res;
  res.reserve(suggestions.size());

  for (const auto& cur : suggestions)
    res.push_back(cur.AsString());

  return res;
}

Filter DefaultFilterEngine::GetAllowlistingFilter(const std::string& url,
                                                  ContentTypeMask contentTypeMask,
                                                  const std::vector<std::string>& documentUrls,
                                                  const std::string& sitekey) const
{
  // WebExt finds allow filters by iterating through parent frames of |url|.
  // https://gitlab.com/eyeo/adblockplus/adblockpluschrome/-/blob/6a345b830841052c09cfce6faf77eb8e682d7b7a/lib/allowlisting.js#L84
  for (auto it = documentUrls.begin(); it != documentUrls.end(); ++it)
  {
    const auto& currentUrl = *it;
    const auto parentIterator = std::next(it);
    auto parentUrl = parentIterator != documentUrls.end() ? *parentIterator : "";
    if (parentUrl.empty())
    {
      // This is the top of the frame hierarchy, pass currentUrl as parent.
      // This is consistent with WebExt ("|| frame.url.hostname"):
      // https://gitlab.com/eyeo/adblockplus/adblockpluschrome/-/blob/6a345b830841052c09cfce6faf77eb8e682d7b7a/lib/allowlisting.js#L53
      parentUrl = currentUrl;
    }
    auto match = CheckFilterMatch(currentUrl, contentTypeMask, parentUrl, sitekey, false);
    if (match.IsValid())
      return match;
  }
  return Filter();
}

void DefaultFilterEngine::AddSubscription(const Subscription& subscription)
{
  const auto* impl =
      static_cast<const DefaultSubscriptionImplementation*>(subscription.Implementation());
  JsValue func = jsEngine.Evaluate("API.addSubscriptionToList");
  func.Call(impl->jsObject);
}

void DefaultFilterEngine::RemoveSubscription(const Subscription& subscription)
{
  const auto* impl =
      static_cast<const DefaultSubscriptionImplementation*>(subscription.Implementation());
  JsValue func = jsEngine.Evaluate("API.removeSubscriptionFromList");
  func.Call(impl->jsObject);
}

void DefaultFilterEngine::AddFilter(const Filter& filter)
{
  if (!filter.IsValid())
    return;
  const auto* impl = static_cast<const DefaultFilterImplementation*>(filter.Implementation());
  JsValue func = jsEngine.Evaluate("API.addFilterToList");
  func.Call(impl->jsObject);
}

void DefaultFilterEngine::RemoveFilter(const Filter& filter)
{
  if (!filter.IsValid())
    return;
  const auto* impl = static_cast<const DefaultFilterImplementation*>(filter.Implementation());
  JsValue func = jsEngine.Evaluate("API.removeFilterFromList");
  func.Call(impl->jsObject);
}

void DefaultFilterEngine::StartObservingEvents()
{
  AddEventObserver(&observer);
}

void DefaultFilterEngine::Observer::OnFilterEvent(FilterEvent event, const Filter&)
{
  if (event == IFilterEngine::FilterEvent::FILTERS_SAVE)
    jsEngine.NotifyLowMemory();
}
