#include <algorithm>
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
JsObject::JsObject(JsValuePtr value)
    : JsValue(value->jsEngine, value->value)
{
  if (!IsObject())
    throw std::runtime_error("JavaScript value is not an object");
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
#else
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsString())
    return value->AsString();
  else
    return defaultValue;
#endif
}

int64_t JsObject::GetProperty(const std::string& name, int64_t defaultValue) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, int64_t>::const_iterator it = intProperties.find(name);
  if (it == intProperties.end())
    return defaultValue;
  else
    return it->second;
#else
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsNumber())
    return value->AsInt();
  else
    return defaultValue;
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
#else
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsBool())
    return value->AsBool();
  else
    return defaultValue;
#endif
}

#if FILTER_ENGINE_STUBS
void JsObject::SetProperty(const std::string& name, const std::string& value)
{
  stringProperties[name] = value;
}

void JsObject::SetProperty(const std::string& name, int64_t value)
{
  intProperties[name] = value;
}

void JsObject::SetProperty(const std::string& name, bool value)
{
  boolProperties[name] = value;
}
#endif

#if FILTER_ENGINE_STUBS
Filter::Filter(FilterEngine& filterEngine, const std::string& text)
    : JsObject(filterEngine)
{
  SetProperty("text", text);

  Type type;
  if (text.find("!") == 0)
    type = TYPE_COMMENT;
  else if (text.find("@@") == 0)
    type = TYPE_EXCEPTION;
  else if (text.find("#@") != std::string::npos)
    type = TYPE_ELEMHIDE_EXCEPTION;
  else if (text.find("#") != std::string::npos)
    type = TYPE_ELEMHIDE;
  else
    type = TYPE_BLOCKING;
  SetProperty("type", type);
}
#else
Filter::Filter(JsValuePtr value)
    : JsObject(value)
{
  // Hack: set `type` property according to class name
  std::string className = GetClassName();
  Type type;
  if (className == "BlockingFilter")
    type = TYPE_BLOCKING;
  else if (className == "WhitelistFilter")
    type = TYPE_EXCEPTION;
  else if (className == "ElemHideFilter")
    type = TYPE_ELEMHIDE;
  else if (className == "ElemHideException")
    type = TYPE_ELEMHIDE_EXCEPTION;
  else if (className == "CommentFilter")
    type = TYPE_COMMENT;
  else
    type = TYPE_INVALID;
  SetProperty("type", type);
}
#endif

bool Filter::IsListed()
{
#if FILTER_ENGINE_STUBS
  for (std::vector<FilterPtr>::iterator it = filterEngine.listedFilters.begin();
       it != filterEngine.listedFilters.end(); ++it)
  {
    if (it->get() == this)
      return true;
  }
  return false;
#else
  JsValuePtr func = jsEngine.Evaluate("API.isListedFilter");
  JsValueList params;
  params.push_back(shared_from_this());
  return func->Call(params)->AsBool();
#endif
}

void Filter::AddToList()
{
#if FILTER_ENGINE_STUBS
  if (!IsListed())
    filterEngine.listedFilters.push_back(shared_from_this());
#else
  JsValuePtr func = jsEngine.Evaluate("API.addFilterToList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
#endif
}

void Filter::RemoveFromList()
{
#if FILTER_ENGINE_STUBS
  for (std::vector<FilterPtr>::iterator it = filterEngine.listedFilters.begin();
       it != filterEngine.listedFilters.end();)
  {
    if (it->get() == this)
      it = filterEngine.listedFilters.erase(it);
    else
      it++;
  }
#else
  JsValuePtr func = jsEngine.Evaluate("API.removeFilterFromList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
#endif
}

bool Filter::operator==(const Filter& filter) const
{
  return GetProperty("text", "") == filter.GetProperty("text", "");
}

#if FILTER_ENGINE_STUBS
Subscription::Subscription(FilterEngine& filterEngine, const std::string& url)
    : JsObject(filterEngine)
{
  SetProperty("url", url);
}
#else
Subscription::Subscription(JsValuePtr value)
    : JsObject(value)
{
}
#endif

bool Subscription::IsListed()
{
#if FILTER_ENGINE_STUBS
  for (std::vector<SubscriptionPtr>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end(); ++it)
  {
    if (it->get() == this)
      return true;
  }
  return false;
#else
  JsValuePtr func = jsEngine.Evaluate("API.isListedFilter");
  JsValueList params;
  params.push_back(shared_from_this());
  return func->Call(params)->AsBool();
#endif
}

void Subscription::AddToList()
{
#if FILTER_ENGINE_STUBS
  if (!IsListed())
    filterEngine.listedSubscriptions.push_back(shared_from_this());
#else
  JsValuePtr func = jsEngine.Evaluate("API.addSubscriptionToList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
#endif
}

void Subscription::RemoveFromList()
{
#if FILTER_ENGINE_STUBS
  for (std::vector<SubscriptionPtr>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end();)
  {
    if (it->get() == this)
      it = filterEngine.listedSubscriptions.erase(it);
    else
      it++;
  }
#else
  JsValuePtr func = jsEngine.Evaluate("API.removeSubscriptionFromList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
#endif
}

void Subscription::UpdateFilters()
{
#if !FILTER_ENGINE_STUBS
  JsValuePtr func = jsEngine.Evaluate("API.updateSubscription");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
#endif
}

bool Subscription::IsUpdating()
{
#if FILTER_ENGINE_STUBS
  return false;
#else
  JsValuePtr func = jsEngine.Evaluate("API.isSubscriptionUpdating");
  JsValueList params;
  params.push_back(shared_from_this());
  JsValuePtr result = func->Call(params);
  return result->AsBool();
#endif
}

bool Subscription::operator==(const Subscription& subscription) const
{
  return GetProperty("url", "") == subscription.GetProperty("url", "");
}

FilterEngine::FilterEngine(JsEngine& jsEngine) : jsEngine(jsEngine)
{
#if !FILTER_ENGINE_STUBS
  for (int i = 0; jsSources[i] && jsSources[i + 1]; i += 2)
    jsEngine.Evaluate(jsSources[i + 1], jsSources[i]);
#endif
}

FilterPtr FilterEngine::GetFilter(const std::string& text)
{
#if FILTER_ENGINE_STUBS
  // Via http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
  std::string trimmed(text);
  trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());

  std::map<std::string, FilterPtr>::const_iterator it = knownFilters.find(trimmed);
  if (it != knownFilters.end())
    return it->second;

  FilterPtr result(new Filter(*this, trimmed));
  knownFilters[trimmed] = result;
  return result;
#else
  JsValuePtr func = jsEngine.Evaluate("API.getFilterFromText");
  JsValueList params;
  params.push_back(jsEngine.NewValue(text));
  return FilterPtr(new Filter(func->Call(params)));
#endif
}

SubscriptionPtr FilterEngine::GetSubscription(const std::string& url)
{
#if FILTER_ENGINE_STUBS
  std::map<std::string, SubscriptionPtr>::const_iterator it = knownSubscriptions.find(url);
  if (it != knownSubscriptions.end())
    return it->second;

  SubscriptionPtr result(new Subscription(*this, url));
  knownSubscriptions[url] = result;
  return result;
#else
  JsValuePtr func = jsEngine.Evaluate("API.getSubscriptionFromUrl");
  JsValueList params;
  params.push_back(jsEngine.NewValue(url));
  return SubscriptionPtr(new Subscription(func->Call(params)));
#endif
}

const std::vector<FilterPtr> FilterEngine::GetListedFilters() const
{
#if FILTER_ENGINE_STUBS
  return listedFilters;
#else
  JsValuePtr func = jsEngine.Evaluate("API.getListedFilters");
  JsValueList values = func->Call()->AsList();
  std::vector<FilterPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(FilterPtr(new Filter(*it)));
  return result;
#endif
}

const std::vector<SubscriptionPtr> FilterEngine::GetListedSubscriptions() const
{
#if FILTER_ENGINE_STUBS
  return listedSubscriptions;
#else
  JsValuePtr func = jsEngine.Evaluate("API.getListedSubscriptions");
  JsValueList values = func->Call()->AsList();
  std::vector<SubscriptionPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(SubscriptionPtr(new Subscription(*it)));
  return result;
#endif
}

void FilterEngine::FetchAvailableSubscriptions(SubscriptionsCallback callback)
{
#if FILTER_ENGINE_STUBS
  std::vector<SubscriptionPtr> availableSubscriptions;

  SubscriptionPtr subscription1 = GetSubscription("https://easylist-downloads.adblockplus.org/easylist.txt");
  subscription1->SetProperty("title", "EasyList");
  availableSubscriptions.push_back(subscription1);

  SubscriptionPtr subscription2 = GetSubscription("https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt");
  subscription2->SetProperty("title", "EasyList Germany+EasyList");
  availableSubscriptions.push_back(subscription2);

  callback(availableSubscriptions);
#else
  // TODO!
#endif
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    const std::string& contentType,
    const std::string& documentUrl)
{
#if FILTER_ENGINE_STUBS
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  if (url.find("adbanner.gif") != std::string::npos)
    return GetFilter("adbanner.gif");
  else if (url.find("notbanner.gif") != std::string::npos)
    return GetFilter("@@notbanner.gif");
  else
    return AdblockPlus::FilterPtr();
#else
  JsValuePtr func = jsEngine.Evaluate("API.checkFilterMatch");
  JsValueList params;
  params.push_back(jsEngine.NewValue(url));
  params.push_back(jsEngine.NewValue(contentType));
  params.push_back(jsEngine.NewValue(documentUrl));
  JsValuePtr result = func->Call(params);
  if (!result->IsNull())
    return FilterPtr(new Filter(result));
  else
    return FilterPtr();
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
#else
  JsValuePtr func = jsEngine.Evaluate("API.getElementHidingSelectors");
  JsValueList params;
  params.push_back(jsEngine.NewValue(domain));
  JsValueList result = func->Call(params)->AsList();
  std::vector<std::string> selectors;
  for (JsValueList::iterator it = result.begin(); it != result.end(); ++it)
    selectors.push_back((*it)->AsString());
  return selectors;
#endif
}
