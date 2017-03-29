/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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

#include <AdblockPlus.h>
#include "JsContext.h"
#include "Thread.h"
#include <mutex>
#include <condition_variable>

using namespace AdblockPlus;

extern std::string jsSources[];

Filter::Filter(JsValue&& value)
    : JsValue(std::move(value))
{
  if (!IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

Filter::Type Filter::GetType() const
{
  std::string className = GetClass();
  if (className == "BlockingFilter")
    return TYPE_BLOCKING;
  else if (className == "WhitelistFilter")
    return TYPE_EXCEPTION;
  else if (className == "ElemHideFilter")
    return TYPE_ELEMHIDE;
  else if (className == "ElemHideException")
    return TYPE_ELEMHIDE_EXCEPTION;
  else if (className == "CommentFilter")
    return TYPE_COMMENT;
  else
    return TYPE_INVALID;
}

bool Filter::IsListed() const
{
  JsValuePtr func = jsEngine->Evaluate("API.isListedFilter");
  return func->Call(*this).AsBool();
}

void Filter::AddToList()
{
  JsValuePtr func = jsEngine->Evaluate("API.addFilterToList");
  func->Call(*this);
}

void Filter::RemoveFromList()
{
  JsValuePtr func = jsEngine->Evaluate("API.removeFilterFromList");
  func->Call(*this);
}

bool Filter::operator==(const Filter& filter) const
{
  return GetProperty("text")->AsString() == filter.GetProperty("text")->AsString();
}

Subscription::Subscription(JsValue&& value)
    : JsValue(std::move(value))
{
  if (!IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

bool Subscription::IsListed() const
{
  JsValuePtr func = jsEngine->Evaluate("API.isListedSubscription");
  return func->Call(*this).AsBool();
}

void Subscription::AddToList()
{
  JsValuePtr func = jsEngine->Evaluate("API.addSubscriptionToList");
  func->Call(*this);
}

void Subscription::RemoveFromList()
{
  JsValuePtr func = jsEngine->Evaluate("API.removeSubscriptionFromList");
  func->Call(*this);
}

void Subscription::UpdateFilters()
{
  JsValuePtr func = jsEngine->Evaluate("API.updateSubscription");
  func->Call(*this);
}

bool Subscription::IsUpdating() const
{
  JsValuePtr func = jsEngine->Evaluate("API.isSubscriptionUpdating");
  return func->Call(*this).AsBool();
}

bool Subscription::operator==(const Subscription& subscription) const
{
  return GetProperty("url")->AsString() == subscription.GetProperty("url")->AsString();
}

namespace
{
  class Sync
  {
  public:
    Sync()
      :initialized(false)
    {

    }
    void Wait()
    {
      std::unique_lock<std::mutex> lock(mutex);
      while (!initialized)
        cv.wait(lock);
    }
    void Set()
    {
      {
        std::unique_lock<std::mutex> lock(mutex);
        initialized = true;
      }
      cv.notify_all();
    }
  private:
    std::mutex mutex;
    std::condition_variable cv;
    bool initialized;
  };
}

FilterEngine::FilterEngine(const JsEnginePtr& jsEngine)
  : jsEngine(jsEngine), firstRun(false), updateCheckId(0)
{
}

void FilterEngine::CreateAsync(const JsEnginePtr& jsEngine,
  const FilterEngine::OnCreatedCallback& onCreated,
  const FilterEngine::CreationParameters& params)
{
  FilterEnginePtr filterEngine(new FilterEngine(jsEngine));
  auto sync = std::make_shared<Sync>();
  auto isConnectionAllowedCallback = params.isConnectionAllowedCallback;
  if (isConnectionAllowedCallback)
    jsEngine->SetIsConnectionAllowedCallback([sync, jsEngine]()->bool
    {
      sync->Wait();
      return jsEngine->IsConnectionAllowed();
    });
  jsEngine->SetEventCallback("_init", [jsEngine, filterEngine, onCreated, sync, isConnectionAllowedCallback](JsValueList& params)
  {
    filterEngine->firstRun = params.size() && params[0]->AsBool();
    if (isConnectionAllowedCallback)
    {
      std::weak_ptr<FilterEngine> weakFilterEngine = filterEngine;
      jsEngine->SetIsConnectionAllowedCallback([weakFilterEngine, isConnectionAllowedCallback]()->bool
      {
        auto filterEngine = weakFilterEngine.lock();
        if (!filterEngine)
          return false;
        return isConnectionAllowedCallback(filterEngine->GetAllowedConnectionType().get());
      });
    }
    sync->Set();
    onCreated(filterEngine);
    jsEngine->RemoveEventCallback("_init");
  });

  // Lock the JS engine while we are loading scripts, no timeouts should fire
  // until we are done.
  const JsContext context(jsEngine);

  // Set the preconfigured prefs
  JsValuePtr preconfiguredPrefsObject = jsEngine->NewObject();
  for (FilterEngine::Prefs::const_iterator it = params.preconfiguredPrefs.begin();
    it != params.preconfiguredPrefs.end(); it++)
  {
    preconfiguredPrefsObject->SetProperty(it->first, it->second);
  }
  jsEngine->SetGlobalProperty("_preconfiguredPrefs", preconfiguredPrefsObject);
  // Load adblockplus scripts
  for (int i = 0; !jsSources[i].empty(); i += 2)
    jsEngine->Evaluate(jsSources[i + 1], jsSources[i]);
}

FilterEnginePtr FilterEngine::Create(const JsEnginePtr& jsEngine,
  const FilterEngine::CreationParameters& params)
{
  FilterEnginePtr retValue;
  Sync sync;
  CreateAsync(jsEngine, [&retValue, &sync](const FilterEnginePtr& filterEngine)
  {
    retValue = filterEngine;
    sync.Set();
  }, params);
  sync.Wait();
  return retValue;
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
    contentTypes[FilterEngine::CONTENT_TYPE_DOCUMENT] = "DOCUMENT";
    contentTypes[FilterEngine::CONTENT_TYPE_PING] = "PING";
    contentTypes[FilterEngine::CONTENT_TYPE_XMLHTTPREQUEST] = "XMLHTTPREQUEST";
    contentTypes[FilterEngine::CONTENT_TYPE_OBJECT_SUBREQUEST] = "OBJECT_SUBREQUEST";
    contentTypes[FilterEngine::CONTENT_TYPE_FONT] = "FONT";
    contentTypes[FilterEngine::CONTENT_TYPE_MEDIA] = "MEDIA";
    contentTypes[FilterEngine::CONTENT_TYPE_ELEMHIDE] = "ELEMHIDE";
    contentTypes[FilterEngine::CONTENT_TYPE_GENERICBLOCK] = "GENERICBLOCK";
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

FilterPtr FilterEngine::GetFilter(const std::string& text) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getFilterFromText");
  return FilterPtr(new Filter(func->Call(*jsEngine->NewValue(text))));
}

SubscriptionPtr FilterEngine::GetSubscription(const std::string& url) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getSubscriptionFromUrl");
  return SubscriptionPtr(new Subscription(func->Call(*jsEngine->NewValue(url))));
}

std::vector<FilterPtr> FilterEngine::GetListedFilters() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getListedFilters");
  JsValueList values = func->Call().AsList();
  std::vector<FilterPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(FilterPtr(new Filter(std::move(**it))));
  return result;
}

std::vector<SubscriptionPtr> FilterEngine::GetListedSubscriptions() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getListedSubscriptions");
  JsValueList values = func->Call().AsList();
  std::vector<SubscriptionPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(SubscriptionPtr(new Subscription(std::move(**it))));
  return result;
}

std::vector<SubscriptionPtr> FilterEngine::FetchAvailableSubscriptions() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getRecommendedSubscriptions");
  JsValueList values = func->Call().AsList();
  std::vector<SubscriptionPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(SubscriptionPtr(new Subscription(std::move(**it))));
  return result;
}

void FilterEngine::ShowNextNotification(const std::string& url)
{
  JsValuePtr func = jsEngine->Evaluate("API.showNextNotification");
  JsConstValueList params;
  if (!url.empty())
  {
    params.push_back(jsEngine->NewValue(url));
  }
  func->Call(params);
}

void FilterEngine::SetShowNotificationCallback(const ShowNotificationCallback& value)
{
  if (!value)
    return;

  jsEngine->SetEventCallback("_showNotification",
    std::bind(&FilterEngine::ShowNotification, this, value,
              std::placeholders::_1));
}

void FilterEngine::RemoveShowNotificationCallback()
{
  jsEngine->RemoveEventCallback("_showNotification");
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::string& documentUrl) const
{
  std::vector<std::string> documentUrls;
  documentUrls.push_back(documentUrl);
  return Matches(url, contentTypeMask, documentUrls);
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::vector<std::string>& documentUrls) const
{
  if (documentUrls.empty())
    return CheckFilterMatch(url, contentTypeMask, "");

  std::string lastDocumentUrl = documentUrls.front();
  for (const auto& documentUrl : documentUrls) {
    AdblockPlus::FilterPtr match = CheckFilterMatch(documentUrl,
                                                    CONTENT_TYPE_DOCUMENT,
                                                    lastDocumentUrl);
    if (match && match->GetType() == AdblockPlus::Filter::TYPE_EXCEPTION)
      return match;
    lastDocumentUrl = documentUrl;
  }

  return CheckFilterMatch(url, contentTypeMask, lastDocumentUrl);
}

bool FilterEngine::IsDocumentWhitelisted(const std::string& url,
    const std::vector<std::string>& documentUrls) const
{
    return !!GetWhitelistingFilter(url, CONTENT_TYPE_DOCUMENT, documentUrls);
}

bool FilterEngine::IsElemhideWhitelisted(const std::string& url,
    const std::vector<std::string>& documentUrls) const
{
    return !!GetWhitelistingFilter(url, CONTENT_TYPE_ELEMHIDE, documentUrls);
}

AdblockPlus::FilterPtr FilterEngine::CheckFilterMatch(const std::string& url,
    ContentTypeMask contentTypeMask,
    const std::string& documentUrl) const
{
  JsValuePtr func = jsEngine->Evaluate("API.checkFilterMatch");
  JsConstValueList params;
  params.push_back(jsEngine->NewValue(url));
  params.push_back(jsEngine->NewValue(contentTypeMask));
  params.push_back(jsEngine->NewValue(documentUrl));
  JsValue result = func->Call(params);
  if (!result.IsNull())
    return FilterPtr(new Filter(std::move(result)));
  else
    return FilterPtr();
}

std::vector<std::string> FilterEngine::GetElementHidingSelectors(const std::string& domain) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getElementHidingSelectors");
  JsValueList result = func->Call(*jsEngine->NewValue(domain)).AsList();
  std::vector<std::string> selectors;
  for (const auto& r: result)
    selectors.push_back(r->AsString());
  return selectors;
}

JsValuePtr FilterEngine::GetPref(const std::string& pref) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getPref");
  return std::make_shared<JsValue>(func->Call(*jsEngine->NewValue(pref)));
}

void FilterEngine::SetPref(const std::string& pref, JsValuePtr value)
{
  JsValuePtr func = jsEngine->Evaluate("API.setPref");
  JsConstValueList params;
  params.push_back(jsEngine->NewValue(pref));
  if (value)
    params.push_back(value);
  func->Call(params);
}

std::string FilterEngine::GetHostFromURL(const std::string& url) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getHostFromUrl");
  return func->Call(*jsEngine->NewValue(url)).AsString();
}

void FilterEngine::SetUpdateAvailableCallback(
    FilterEngine::UpdateAvailableCallback callback)
{
  jsEngine->SetEventCallback("updateAvailable",
      std::bind(&FilterEngine::UpdateAvailable, this, callback,
                std::placeholders::_1));
}

void FilterEngine::RemoveUpdateAvailableCallback()
{
  jsEngine->RemoveEventCallback("updateAvailable");
}

void FilterEngine::UpdateAvailable(
    FilterEngine::UpdateAvailableCallback callback, JsValueList& params)
{
  if (params.size() >= 1 && !params[0]->IsNull())
    callback(params[0]->AsString());
}

void FilterEngine::ForceUpdateCheck(
    const FilterEngine::UpdateCheckDoneCallback& callback)
{
  JsValuePtr func = jsEngine->Evaluate("API.forceUpdateCheck");
  JsConstValueList params;
  if (callback)
  {
    std::string eventName = "_updateCheckDone" + std::to_string(++updateCheckId);
    jsEngine->SetEventCallback(eventName, std::bind(&FilterEngine::UpdateCheckDone,
      this, eventName, callback, std::placeholders::_1));
    params.push_back(jsEngine->NewValue(eventName));
  }
  func->Call(params);
}

void FilterEngine::UpdateCheckDone(const std::string& eventName,
    FilterEngine::UpdateCheckDoneCallback callback, JsValueList& params)
{
  jsEngine->RemoveEventCallback(eventName);

  std::string error(params.size() >= 1 && !params[0]->IsNull() ? params[0]->AsString() : "");
  callback(error);
}

void FilterEngine::SetFilterChangeCallback(FilterEngine::FilterChangeCallback callback)
{
  jsEngine->SetEventCallback("filterChange", std::bind(&FilterEngine::FilterChanged,
      this, callback, std::placeholders::_1));
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
   if (prefValue->AsString().empty())
     return nullptr;
   return std::unique_ptr<std::string>(new std::string(prefValue->AsString()));
}

void FilterEngine::FilterChanged(FilterEngine::FilterChangeCallback callback, JsValueList& params)
{
  std::string action(params.size() >= 1 && !params[0]->IsNull() ? params[0]->AsString() : "");
  JsValuePtr item(params.size() >= 2 ? params[1] : jsEngine->NewValue(false));
  callback(action, item);
}

void FilterEngine::ShowNotification(const ShowNotificationCallback& callback,
                                         const JsValueList& params)
{
  if (params.size() < 1)
    return;

  if (!params[0]->IsObject())
  {
    return;
  }
  callback(NotificationPtr(new Notification(std::move(*params[0]))));
}


int FilterEngine::CompareVersions(const std::string& v1, const std::string& v2) const
{
  JsConstValueList params;
  params.push_back(jsEngine->NewValue(v1));
  params.push_back(jsEngine->NewValue(v2));
  JsValuePtr func = jsEngine->Evaluate("API.compareVersions");
  return func->Call(params).AsInt();
}

FilterPtr FilterEngine::GetWhitelistingFilter(const std::string& url,
  ContentTypeMask contentTypeMask, const std::string& documentUrl) const
{
  FilterPtr match = Matches(url, contentTypeMask, documentUrl);
  if (match && match->GetType() == Filter::TYPE_EXCEPTION)
  {
    return match;
  }
  return FilterPtr();
}

FilterPtr FilterEngine::GetWhitelistingFilter(const std::string& url,
  ContentTypeMask contentTypeMask,
  const std::vector<std::string>& documentUrls) const
{
  if (documentUrls.empty())
  {
    return GetWhitelistingFilter(url, contentTypeMask, "");
  }

  std::vector<std::string>::const_iterator urlIterator = documentUrls.begin();
  std::string currentUrl = url;
  do
  {
    std::string parentUrl = *urlIterator++;
    FilterPtr filter = GetWhitelistingFilter(currentUrl, contentTypeMask, parentUrl);
    if (filter)
    {
      return filter;
    }
    currentUrl = parentUrl;
  }
  while (urlIterator != documentUrls.end());
  return FilterPtr();
}
