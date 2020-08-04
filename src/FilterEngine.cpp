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

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <cassert>
#include <thread>

#include <AdblockPlus.h>
#include "JsContext.h"
#include "Thread.h"
#include <mutex>
#include <condition_variable>

using namespace AdblockPlus;

namespace
{
    const std::string filterEngineJsFiles[] =
    {
      "compat.js",
      "info.js",
      "io.js",
      "prefs.js",
      "utils.js",
      "elemHideHitRegistration.js",
      "time.js",
      "events.js",
      "coreUtils.js",
      "caching.js",
      "publicSuffixList.json",
      "subscriptions.json",
      "resources.json",
      "url.js",
      "filterNotifier.js",
      "recommendations.js",
      "common.js",
      "elemHideExceptions.js",
      "contentTypes.js",
      "filterClasses.js",
      "snippets.js",
      "analytics.js",
      "downloader.js",
      "subscriptionClasses.js",
      "iniParser.js",
      "filterStorage.js",
      "filtersByDomain.js",
      "elemHide.js",
      "elemHideEmulation.js",
      "matcher.js",
      "filterListener.js",
      "filterEngine.js",
      "versions.js",
      "notifications.js",
      "notificationShowRegistration.js",
      "synchronizer.js",
      "filterUpdateRegistration.js",
      "jsbn.js",
      "rusha.js",
      "rsa.js",
      "init.js",
      "api.js",
      "punycode.js",
      "uri.js"
    };
}

FilterEngine::FilterEngine(const JsEnginePtr& jsEngine)
  : jsEngine(jsEngine), firstRun(false)
{

}

void FilterEngine::CreateAsync(const JsEnginePtr& jsEngine,
  const EvaluateCallback& evaluateCallback,
  const FilterEngine::OnCreatedCallback& onCreated,
  const FilterEngine::CreationParameters& params)
{
  FilterEnginePtr filterEngine(new FilterEngine(jsEngine));
  {
    // TODO: replace weakFilterEngine by this when it's possible to control the
    // execution time of the asynchronous part below.
    std::weak_ptr<FilterEngine> weakFilterEngine = filterEngine;
    auto isSubscriptionDownloadAllowedCallback = params.isSubscriptionDownloadAllowedCallback;
    jsEngine->SetEventCallback("_isSubscriptionDownloadAllowed", [weakFilterEngine, isSubscriptionDownloadAllowedCallback](JsValueList&& params){
      auto filterEngine = weakFilterEngine.lock();
      if (!filterEngine)
        return;
      auto& jsEngine = filterEngine->GetJsEngine();

      // param[0] - nullable string Prefs.allowed_connection_type
      // param[1] - function(Boolean)
      bool areArgumentsValid = params.size() == 2 && (params[0].IsNull() || params[0].IsString()) && params[1].IsFunction();
      assert(areArgumentsValid && "Invalid argument: there should be two args and the second one should be a function");
      if (!areArgumentsValid)
        return;
      if (!isSubscriptionDownloadAllowedCallback)
      {
        params[1].Call(jsEngine.NewValue(true));
        return;
      }
      auto valuesID = jsEngine.StoreJsValues(params);
      auto callJsCallback = [weakFilterEngine, valuesID](bool isAllowed)
      {
        auto filterEngine = weakFilterEngine.lock();
        if (!filterEngine)
          return;
        auto& jsEngine = filterEngine->GetJsEngine();
        auto jsParams = jsEngine.TakeJsValues(valuesID);
        jsParams[1].Call(jsEngine.NewValue(isAllowed));
      };
      std::string allowedConnectionType = params[0].IsString() ? params[0].AsString() : std::string();
      isSubscriptionDownloadAllowedCallback(params[0].IsString() ? &allowedConnectionType : nullptr, callJsCallback);
    });
  }

  jsEngine->SetEventCallback("_init", [jsEngine, filterEngine, onCreated](JsValueList&& params)
  {
    filterEngine->firstRun = params.size() && params[0].AsBool();
    onCreated(filterEngine);
    jsEngine->RemoveEventCallback("_init");
  });

  std::weak_ptr<FilterEngine> weakFilterEngine = filterEngine;
  filterEngine->SetFilterChangeCallback([weakFilterEngine](const std::string& reason, JsValue&&)
  {
    auto filterEngine = weakFilterEngine.lock();
    if (!filterEngine)
      return;
    if (reason == "save")
      filterEngine->GetJsEngine().NotifyLowMemory();
  });

  // Lock the JS engine while we are loading scripts, no timeouts should fire
  // until we are done.
  const JsContext context(*jsEngine);
  // Set the preconfigured prefs
  auto preconfiguredPrefsObject = jsEngine->NewObject();
  for (const auto& pref : params.preconfiguredPrefs)
  {
    preconfiguredPrefsObject.SetProperty(pref.first, pref.second);
  }
  jsEngine->SetGlobalProperty("_preconfiguredPrefs", preconfiguredPrefsObject);

  // Load adblockplus scripts
  for (const auto& filterEngineJsFile: filterEngineJsFiles)
    evaluateCallback(filterEngineJsFile);

}

namespace
{
  typedef std::map<FilterEngine::ContentType, std::string> ContentTypeMap;

  ContentTypeMap CreateContentTypeMap()
  {
    ContentTypeMap contentTypes;
    contentTypes[FilterEngine::CONTENT_TYPE_OTHER] = "OTHER";
    contentTypes[FilterEngine::CONTENT_TYPE_SCRIPT] = "SCRIPT";
    contentTypes[FilterEngine::CONTENT_TYPE_IMAGE] = "IMAGE";
    contentTypes[FilterEngine::CONTENT_TYPE_STYLESHEET] = "STYLESHEET";
    contentTypes[FilterEngine::CONTENT_TYPE_OBJECT] = "OBJECT";
    contentTypes[FilterEngine::CONTENT_TYPE_SUBDOCUMENT] = "SUBDOCUMENT";
    contentTypes[FilterEngine::CONTENT_TYPE_WEBSOCKET] = "WEBSOCKET";
    contentTypes[FilterEngine::CONTENT_TYPE_WEBRTC] = "WEBRTC";
    contentTypes[FilterEngine::CONTENT_TYPE_PING] = "PING";
    contentTypes[FilterEngine::CONTENT_TYPE_XMLHTTPREQUEST] = "XMLHTTPREQUEST";
    contentTypes[FilterEngine::CONTENT_TYPE_FONT] = "FONT";
    contentTypes[FilterEngine::CONTENT_TYPE_MEDIA] = "MEDIA";
    contentTypes[FilterEngine::CONTENT_TYPE_POPUP] = "POPUP";
    contentTypes[FilterEngine::CONTENT_TYPE_DOCUMENT] = "DOCUMENT";
    contentTypes[FilterEngine::CONTENT_TYPE_GENERICBLOCK] = "GENERICBLOCK";
    contentTypes[FilterEngine::CONTENT_TYPE_ELEMHIDE] = "ELEMHIDE";
    contentTypes[FilterEngine::CONTENT_TYPE_GENERICHIDE] = "GENERICHIDE";
    return contentTypes;
  }
}

const ContentTypeMap FilterEngine::contentTypes = CreateContentTypeMap();

std::string FilterEngine::ContentTypeToString(ContentType contentType)
{
  ContentTypeMap::const_iterator it = contentTypes.find(contentType);
  if (it != contentTypes.end())
    return it->second;
  throw std::invalid_argument("Argument is not a valid ContentType");
}

FilterEngine::ContentType FilterEngine::StringToContentType(const std::string& contentType)
{
  std::string contentTypeUpper = contentType;
  std::transform(contentType.begin(), contentType.end(), contentTypeUpper.begin(), ::toupper);
  for (const auto& contentType : contentTypes)
  {
    if (contentType.second == contentTypeUpper)
      return contentType.first;
  }
  throw std::invalid_argument("Cannot convert argument to ContentType");
}

bool FilterEngine::IsFirstRun() const
{
  return firstRun;
}

Filter FilterEngine::GetFilter(const std::string& text) const
{
  JsValue func = jsEngine->Evaluate("API.getFilterFromText");
  return Filter(func.Call(jsEngine->NewValue(text)));
}

Subscription FilterEngine::GetSubscription(const std::string& url) const
{
  JsValue func = jsEngine->Evaluate("API.getSubscriptionFromUrl");
  return Subscription(func.Call(jsEngine->NewValue(url)));
}

std::vector<Filter> FilterEngine::GetListedFilters() const
{
  JsValue func = jsEngine->Evaluate("API.getListedFilters");
  JsValueList values = func.Call().AsList();
  std::vector<Filter> result;
  for (auto& value : values)
    result.push_back(Filter(std::move(value)));
  return result;
}

std::vector<Subscription> FilterEngine::GetListedSubscriptions() const
{
  JsValue func = jsEngine->Evaluate("API.getListedSubscriptions");
  JsValueList values = func.Call().AsList();
  std::vector<Subscription> result;
  for (auto& value : values)
    result.push_back(Subscription(std::move(value)));
  return result;
}

std::vector<Subscription> FilterEngine::FetchAvailableSubscriptions() const
{
  JsValue func = jsEngine->Evaluate("API.getRecommendedSubscriptions");
  JsValueList values = func.Call().AsList();
  std::vector<Subscription> result;
  for (auto& value : values)
    result.push_back(Subscription(std::move(value)));
  return result;
}

void FilterEngine::SetAAEnabled(bool enabled)
{
  jsEngine->Evaluate("API.setAASubscriptionEnabled").Call(jsEngine->NewValue(enabled));
}

bool FilterEngine::IsAAEnabled() const
{
  return jsEngine->Evaluate("API.isAASubscriptionEnabled()").AsBool();
}

std::string FilterEngine::GetAAUrl() const
{
  return GetPref("subscriptions_exceptionsurl").AsString();
}

void FilterEngine::ShowNextNotification() const
{
  jsEngine->Evaluate("API.showNextNotification").Call();
}

void FilterEngine::SetShowNotificationCallback(const ShowNotificationCallback& callback)
{
  if (!callback)
    return;

  jsEngine->SetEventCallback("_showNotification", [callback](JsValueList&& params)
  {
    if (params.size() < 1 || !params[0].IsObject())
      return;

    callback(Notification(std::move(params[0])));
  });
}

void FilterEngine::RemoveShowNotificationCallback()
{
  jsEngine->RemoveEventCallback("_showNotification");
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::string& documentUrl,
    const std::string& siteKey,
    bool specificOnly) const
{
  std::vector<std::string> documentUrls;
  documentUrls.push_back(documentUrl);
  return Matches(url, contentTypeMask, documentUrls, siteKey, specificOnly);
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::vector<std::string>& documentUrls,
    const std::string& siteKey,
    bool specificOnly) const
{
  if (documentUrls.empty())
    return CheckFilterMatch(url, contentTypeMask, "", siteKey, specificOnly);

  std::string lastDocumentUrl = documentUrls.front();
  for (const auto& documentUrl : documentUrls) {
    AdblockPlus::FilterPtr match = CheckFilterMatch(documentUrl,
                                                    CONTENT_TYPE_DOCUMENT,
                                                    lastDocumentUrl,
                                                    siteKey,
                                                    specificOnly);
    if (match && match->GetType() == AdblockPlus::Filter::TYPE_EXCEPTION)
      return match;
    lastDocumentUrl = documentUrl;
  }

  return CheckFilterMatch(url, contentTypeMask, lastDocumentUrl, siteKey, specificOnly);
}

bool FilterEngine::IsGenericblockWhitelisted(const std::string& url,
    const std::vector<std::string>& documentUrls, const std::string& sitekey) const
{
  return !!GetWhitelistingFilter(url, CONTENT_TYPE_GENERICBLOCK, documentUrls, sitekey);
}

bool FilterEngine::IsDocumentWhitelisted(const std::string& url,
    const std::vector<std::string>& documentUrls,
    const std::string& sitekey) const
{
    return !!GetWhitelistingFilter(url, CONTENT_TYPE_DOCUMENT, documentUrls, sitekey);
}

bool FilterEngine::IsElemhideWhitelisted(const std::string& url,
    const std::vector<std::string>& documentUrls, const std::string& sitekey) const
{
    return !!GetWhitelistingFilter(url, CONTENT_TYPE_ELEMHIDE, documentUrls, sitekey);
}

AdblockPlus::FilterPtr FilterEngine::CheckFilterMatch(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::string& documentUrl,
    const std::string& siteKey,
    bool specificOnly) const
{
  if (url.empty())
    return FilterPtr();
  JsValue func = jsEngine->Evaluate("API.checkFilterMatch");
  JsValueList params;
  params.push_back(jsEngine->NewValue(url));
  params.push_back(jsEngine->NewValue(contentTypeMask));
  params.push_back(jsEngine->NewValue(documentUrl));
  params.push_back(jsEngine->NewValue(siteKey));
  params.push_back(jsEngine->NewValue(specificOnly));
  JsValue result = func.Call(params);
  if (!result.IsNull())
    return FilterPtr(new Filter(std::move(result)));
  else
    return FilterPtr();
}

std::string FilterEngine::GetElementHidingStyleSheet(const std::string& domain, bool specificOnly) const
{
  JsValueList params;
  params.push_back(jsEngine->NewValue(domain));
  params.push_back(jsEngine->NewValue(specificOnly));
  JsValue func = jsEngine->Evaluate("API.getElementHidingStyleSheet");
  return func.Call(params).AsString();
}

std::vector<FilterEngine::EmulationSelector> FilterEngine::GetElementHidingEmulationSelectors(const std::string& domain) const
{
  JsValue func = jsEngine->Evaluate("API.getElementHidingEmulationSelectors");
  JsValueList result = func.Call(jsEngine->NewValue(domain)).AsList();
  std::vector<FilterEngine::EmulationSelector> selectors;
  selectors.reserve(result.size());
  for (const auto& r : result)
    selectors.push_back({r.GetProperty("selector").AsString(), r.GetProperty("text").AsString()});
  return selectors;
}

JsValue FilterEngine::GetPref(const std::string& pref) const
{
  JsValue func = jsEngine->Evaluate("API.getPref");
  return func.Call(jsEngine->NewValue(pref));
}

void FilterEngine::SetPref(const std::string& pref, const JsValue& value)
{
  JsValue func = jsEngine->Evaluate("API.setPref");
  JsValueList params;
  params.push_back(jsEngine->NewValue(pref));
  params.push_back(value);
  func.Call(params);
}

std::string FilterEngine::GetHostFromURL(const std::string& url) const
{
  JsValue func = jsEngine->Evaluate("API.getHostFromUrl");
  return func.Call(jsEngine->NewValue(url)).AsString();
}

void FilterEngine::SetFilterChangeCallback(const FilterChangeCallback& callback)
{
  jsEngine->SetEventCallback("filterChange", [this, callback](JsValueList&& params)
  {
    this->FilterChanged(callback, move(params));
  });
}

void FilterEngine::RemoveFilterChangeCallback()
{
  jsEngine->RemoveEventCallback("filterChange");
}

void FilterEngine::SetAllowedConnectionType(const std::string* value)
{
  SetPref("allowed_connection_type", value ? jsEngine->NewValue(*value) : jsEngine->NewValue(""));
}

std::unique_ptr<std::string> FilterEngine::GetAllowedConnectionType() const
{
   auto prefValue = GetPref("allowed_connection_type");
   if (prefValue.AsString().empty())
     return nullptr;
   return std::unique_ptr<std::string>(new std::string(prefValue.AsString()));
}

void FilterEngine::FilterChanged(const FilterEngine::FilterChangeCallback& callback, JsValueList&& params) const
{
  std::string action(params.size() >= 1 && !params[0].IsNull() ? params[0].AsString() : "");
  JsValue item(params.size() >= 2 ? params[1] : jsEngine->NewValue(false));
  callback(action, std::move(item));
}

int FilterEngine::CompareVersions(const std::string& v1, const std::string& v2) const
{
  JsValueList params;
  params.push_back(jsEngine->NewValue(v1));
  params.push_back(jsEngine->NewValue(v2));
  JsValue func = jsEngine->Evaluate("API.compareVersions");
  return func.Call(params).AsInt();
}

bool FilterEngine::VerifySignature(const std::string& key, const std::string& signature, const std::string& uri,
                                   const std::string& host, const std::string& userAgent) const
{
  JsValueList params;
  params.push_back(jsEngine->NewValue(key));
  params.push_back(jsEngine->NewValue(signature));
  params.push_back(jsEngine->NewValue(uri));
  params.push_back(jsEngine->NewValue(host));
  params.push_back(jsEngine->NewValue(userAgent));
  JsValue func = jsEngine->Evaluate("API.verifySignature");
  return func.Call(params).AsBool();
}

FilterPtr FilterEngine::GetWhitelistingFilter(const std::string& url,
  ContentTypeMask contentTypeMask, const std::string& documentUrl,
  const std::string& sitekey) const
{
  FilterPtr match = Matches(url, contentTypeMask, documentUrl, sitekey);
  if (match && match->GetType() == Filter::TYPE_EXCEPTION)
  {
    return match;
  }
  return FilterPtr();
}

FilterPtr FilterEngine::GetWhitelistingFilter(const std::string& url,
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
    FilterPtr filter = GetWhitelistingFilter(currentUrl, contentTypeMask, parentUrl, sitekey);
    if (filter)
    {
      return filter;
    }
    currentUrl = parentUrl;
  }
  while (urlIterator != documentUrls.end());
  return GetWhitelistingFilter(currentUrl, contentTypeMask, "", sitekey);
}
