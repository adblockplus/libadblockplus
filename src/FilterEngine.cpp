#include <algorithm>
#include <memory>
#include <cctype>
#include <functional>

#include <AdblockPlus.h>

using namespace AdblockPlus;

#if !FILTER_ENGINE_STUBS
extern const char* jsSources[];
#endif

#if FILTER_ENGINE_STUBS
JsObject::JsObject(FilterEngine& filterEngine)
    : filterEngine(filterEngine)
{
}
#else
JsObject::JsObject()
{
}
#endif

std::string JsObject::GetProperty(const std::string& name, const std::string& defaultValue) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, std::string>::const_iterator it = stringProperties.find(name);
  if (it == stringProperties.end())
    return defaultValue;
  else
    return it->second;
#endif
}

int JsObject::GetProperty(const std::string& name, int defaultValue) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, int>::const_iterator it = intProperties.find(name);
  if (it == intProperties.end())
    return defaultValue;
  else
    return it->second;
#endif
}

bool JsObject::GetProperty(const std::string& name, bool defaultValue) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, bool>::const_iterator it = boolProperties.find(name);
  if (it == boolProperties.end())
    return defaultValue;
  else
    return it->second;
#endif
}

void JsObject::SetProperty(const std::string& name, const std::string& value)
{
#if FILTER_ENGINE_STUBS
  stringProperties[name] = value;
#endif
}

void JsObject::SetProperty(const std::string& name, int value)
{
#if FILTER_ENGINE_STUBS
  intProperties[name] = value;
#endif
}

void JsObject::SetProperty(const std::string& name, bool value)
{
#if FILTER_ENGINE_STUBS
  boolProperties[name] = value;
#endif
}

#if FILTER_ENGINE_STUBS
Filter::Filter(FilterEngine& filterEngine, const std::string& text)
    : JsObject(filterEngine)
{
  SetProperty("text", text);
  if (text.find("!") == 0)
    SetProperty("type", COMMENT_RULE);
  else if (text.find("@@") == 0)
    SetProperty("type", EXCEPTION_RULE);
  else if (text.find("#@") != std::string::npos)
    SetProperty("type", ELEMHIDE_EXCEPTION_RULE);
  else if (text.find("#") != std::string::npos)
    SetProperty("type", ELEMHIDE_RULE);
  else
    SetProperty("type", BLOCKING_RULE);
}
#else
Filter::Filter()
{
}
#endif

bool Filter::IsListed() const
{
#if FILTER_ENGINE_STUBS
  for (std::vector<FilterPtr>::iterator it = filterEngine.listedFilters.begin();
       it != filterEngine.listedFilters.end(); ++it)
  {
    if (it->get() == this)
      return true;
  }
  return false;
#endif
}

void Filter::AddToList()
{
#if FILTER_ENGINE_STUBS
  if (!IsListed())
    filterEngine.listedFilters.push_back(shared_from_this());
#endif
}

void Filter::RemoveFromList()
{
  for (std::vector<FilterPtr>::iterator it = filterEngine.listedFilters.begin();
       it != filterEngine.listedFilters.end();)
  {
    if (it->get() == this)
      it = filterEngine.listedFilters.erase(it);
    else
      it++;
  }
}

#if FILTER_ENGINE_STUBS
Subscription::Subscription(FilterEngine& filterEngine, const std::string& url)
    : JsObject(filterEngine)
{
  SetProperty("url", url);
}
#else
Subscription::Subscription()
{
}
#endif

bool Subscription::IsListed() const
{
#if FILTER_ENGINE_STUBS
  for (std::vector<SubscriptionPtr>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end(); ++it)
  {
    if (it->get() == this)
      return true;
  }
  return false;
#endif
}

void Subscription::AddToList()
{
#if FILTER_ENGINE_STUBS
  if (!IsListed())
    filterEngine.listedSubscriptions.push_back(shared_from_this());
#endif
}

void Subscription::RemoveFromList()
{
  for (std::vector<SubscriptionPtr>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end();)
  {
    if (it->get() == this)
      it = filterEngine.listedSubscriptions.erase(it);
    else
      it++;
  }
}

void Subscription::UpdateFilters()
{
}

FilterEngine::FilterEngine(JsEngine& jsEngine) : jsEngine(jsEngine)
{
#if !FILTER_ENGINE_STUBS
  for (int i = 0; jsSources[i] && jsSources[i + 1]; i += 2)
    jsEngine.Evaluate(jsSources[i + 1], jsSources[i]);
#endif
}

Filter& FilterEngine::GetFilter(const std::string& text)
{
#if FILTER_ENGINE_STUBS
  // Via http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
  std::string trimmed(text);
  trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());

  std::map<std::string, FilterPtr>::const_iterator it = knownFilters.find(trimmed);
  if (it != knownFilters.end())
    return *it->second;

  FilterPtr result(new Filter(*this, trimmed));
  knownFilters[trimmed] = result->shared_from_this();
  return *result;
#endif
}

Subscription& FilterEngine::GetSubscription(const std::string& url)
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, SubscriptionPtr>::const_iterator it = knownSubscriptions.find(url);
  if (it != knownSubscriptions.end())
    return *it->second;

  SubscriptionPtr result(new Subscription(*this, url));
  knownSubscriptions[url] = result->shared_from_this();
  return *result;
#endif
}

const std::vector<FilterPtr>& FilterEngine::GetListedFilters() const
{
#if FILTER_ENGINE_STUBS
  return listedFilters;
#endif
}

const std::vector<SubscriptionPtr>& FilterEngine::GetListedSubscriptions() const
{
#if FILTER_ENGINE_STUBS
  return listedSubscriptions;
#endif
}

void FilterEngine::FetchAvailableSubscriptions(SubscriptionsCallback callback)
{
#if FILTER_ENGINE_STUBS
  std::vector<SubscriptionPtr> availableSubscriptions;

  Subscription& subscription1 = GetSubscription("https://easylist-downloads.adblockplus.org/easylist.txt");
  subscription1.SetProperty("title", "EasyList");
  availableSubscriptions.push_back(subscription1.shared_from_this());

  Subscription& subscription2 = GetSubscription("https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt");
  subscription2.SetProperty("title", "EasyList Germany+EasyList");
  availableSubscriptions.push_back(subscription2.shared_from_this());

  callback(availableSubscriptions);
#endif
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    const std::string& contentType,
    const std::string& documentUrl)
{
#if FILTER_ENGINE_STUBS
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  if (url.find("adbanner.gif") != std::string::npos)
    return GetFilter("adbanner.gif").shared_from_this();
  else if (url.find("notbanner.gif") != std::string::npos)
    return GetFilter("@@notbanner.gif").shared_from_this();
  else
    return AdblockPlus::FilterPtr();
#endif
}

std::vector<std::string> FilterEngine::GetElementHidingSelectors(const std::string& domain) const
{
#if FILTER_ENGINE_STUBS
  std::vector<std::string> selectors;
  selectors.push_back("#ad");
  selectors.push_back(".ad");
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  if (domain == "simple-adblock.com")
    selectors.push_back(".ad_300x250");
  return selectors;
#endif
}
