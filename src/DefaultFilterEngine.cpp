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
#include <functional>
#include <string>

#include "DefaultFilterImplementation.h"
#include "DefaultSubscriptionImplementation.h"
#include "ElementUtils.h"
#include "JsContext.h"

using namespace AdblockPlus;

DefaultFilterEngine::DefaultFilterEngine(JsEngine& jsEngine) : jsEngine(jsEngine), firstRun(false)
{
}

bool DefaultFilterEngine::IsFirstRun() const
{
  return firstRun;
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

AdblockPlus::Filter DefaultFilterEngine::Matches(const std::string& url,
                                                 ContentTypeMask contentTypeMask,
                                                 const std::string& documentUrl,
                                                 const std::string& siteKey,
                                                 bool specificOnly) const
{
  std::vector<std::string> documentUrls;
  documentUrls.push_back(documentUrl);
  return Matches(url, contentTypeMask, documentUrls, siteKey, specificOnly);
}

AdblockPlus::Filter DefaultFilterEngine::Matches(const std::string& url,
                                                 ContentTypeMask contentTypeMask,
                                                 const std::vector<std::string>& documentUrls,
                                                 const std::string& siteKey,
                                                 bool specificOnly) const
{
  if (documentUrls.empty())
    return CheckFilterMatch(url, contentTypeMask, "", siteKey, specificOnly);

  std::string lastDocumentUrl = documentUrls.front();
  for (const auto& documentUrl : documentUrls)
  {
    auto match = CheckFilterMatch(
        documentUrl, CONTENT_TYPE_DOCUMENT, lastDocumentUrl, siteKey, specificOnly);
    if (match.IsValid() && match.GetType() == AdblockPlus::IFilterImplementation::TYPE_EXCEPTION)
      return match;
    lastDocumentUrl = documentUrl;
  }

  return CheckFilterMatch(url, contentTypeMask, lastDocumentUrl, siteKey, specificOnly);
}

bool DefaultFilterEngine::IsGenericblockWhitelisted(const std::string& url,
                                                    const std::vector<std::string>& documentUrls,
                                                    const std::string& sitekey) const
{
  return GetWhitelistingFilter(url, CONTENT_TYPE_GENERICBLOCK, documentUrls, sitekey).IsValid();
}

bool DefaultFilterEngine::IsDocumentWhitelisted(const std::string& url,
                                                const std::vector<std::string>& documentUrls,
                                                const std::string& sitekey) const
{
  return GetWhitelistingFilter(url, CONTENT_TYPE_DOCUMENT, documentUrls, sitekey).IsValid();
}

bool DefaultFilterEngine::IsElemhideWhitelisted(const std::string& url,
                                                const std::vector<std::string>& documentUrls,
                                                const std::string& sitekey) const
{
  return GetWhitelistingFilter(url, CONTENT_TYPE_ELEMHIDE, documentUrls, sitekey).IsValid();
}

AdblockPlus::Filter DefaultFilterEngine::CheckFilterMatch(const std::string& url,
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
  JsValueList params;
  params.push_back(jsEngine.NewValue(domain));
  params.push_back(jsEngine.NewValue(specificOnly));
  JsValue func = jsEngine.Evaluate("API.getElementHidingStyleSheet");
  return func.Call(params).AsString();
}

std::vector<IFilterEngine::EmulationSelector>
DefaultFilterEngine::GetElementHidingEmulationSelectors(const std::string& domain) const
{
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
  jsEngine.SetEventCallback("filterChange", [this, callback](JsValueList&& params) {
    this->FilterChanged(callback, move(params));
  });
}

void DefaultFilterEngine::RemoveFilterChangeCallback()
{
  jsEngine.RemoveEventCallback("filterChange");
}

void DefaultFilterEngine::SetAllowedConnectionType(const std::string* value)
{
  SetPref("allowed_connection_type", value ? jsEngine.NewValue(*value) : jsEngine.NewValue(""));
}

std::unique_ptr<std::string> DefaultFilterEngine::GetAllowedConnectionType() const
{
  auto prefValue = GetPref("allowed_connection_type");
  if (prefValue.AsString().empty())
    return nullptr;
  return std::unique_ptr<std::string>(new std::string(prefValue.AsString()));
}

void DefaultFilterEngine::FilterChanged(const IFilterEngine::FilterChangeCallback& callback,
                                        JsValueList&& params) const
{
  std::string action(params.size() >= 1 && !params[0].IsNull() ? params[0].AsString() : "");
  JsValue item(params.size() >= 2 ? params[1] : jsEngine.NewValue(false));
  callback(action, std::move(item));
}

int DefaultFilterEngine::CompareVersions(const std::string& v1, const std::string& v2) const
{
  JsValueList params;
  params.push_back(jsEngine.NewValue(v1));
  params.push_back(jsEngine.NewValue(v2));
  JsValue func = jsEngine.Evaluate("API.compareVersions");
  return func.Call(params).AsInt();
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

Filter DefaultFilterEngine::GetWhitelistingFilter(const std::string& url,
                                                  ContentTypeMask contentTypeMask,
                                                  const std::string& documentUrl,
                                                  const std::string& sitekey) const
{
  auto match = Matches(url, contentTypeMask, documentUrl, sitekey);
  if (match.IsValid() && match.GetType() == IFilterImplementation::TYPE_EXCEPTION)
  {
    return match;
  }
  return Filter();
}

Filter DefaultFilterEngine::GetWhitelistingFilter(const std::string& url,
                                                  ContentTypeMask contentTypeMask,
                                                  const std::vector<std::string>& documentUrls,
                                                  const std::string& sitekey) const
{
  if (documentUrls.empty())
  {
    return GetWhitelistingFilter(url, contentTypeMask, "", sitekey);
  }

  std::vector<std::string>::const_iterator urlIterator = documentUrls.begin();
  std::string currentUrl = url;
  do
  {
    std::string parentUrl = *urlIterator++;
    Filter filter = GetWhitelistingFilter(currentUrl, contentTypeMask, parentUrl, sitekey);
    if (filter.IsValid())
    {
      return filter;
    }
    currentUrl = parentUrl;
  } while (urlIterator != documentUrls.end());
  return GetWhitelistingFilter(currentUrl, contentTypeMask, "", sitekey);
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
