#include <algorithm>
#include <cctype>
#include <functional>

#include <AdblockPlus.h>

using namespace AdblockPlus;

extern const char* jsSources[];

JsObject::JsObject(JsValuePtr value)
    : JsValue(value->jsEngine, value->value)
{
  if (!IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

std::string JsObject::GetProperty(const std::string& name, const std::string& defaultValue) const
{
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsString())
    return value->AsString();
  else
    return defaultValue;
}

int64_t JsObject::GetProperty(const std::string& name, int64_t defaultValue) const
{
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsNumber())
    return value->AsInt();
  else
    return defaultValue;
}

bool JsObject::GetProperty(const std::string& name, bool defaultValue) const
{
  JsValuePtr value = JsValue::GetProperty(name);
  if (value->IsBool())
    return value->AsBool();
  else
    return defaultValue;
}

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

bool Filter::IsListed()
{
  JsValuePtr func = jsEngine->Evaluate("API.isListedFilter");
  JsValueList params;
  params.push_back(shared_from_this());
  return func->Call(params)->AsBool();
}

void Filter::AddToList()
{
  JsValuePtr func = jsEngine->Evaluate("API.addFilterToList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
}

void Filter::RemoveFromList()
{
  JsValuePtr func = jsEngine->Evaluate("API.removeFilterFromList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
}

bool Filter::operator==(const Filter& filter) const
{
  return GetProperty("text", "") == filter.GetProperty("text", "");
}

Subscription::Subscription(JsValuePtr value)
    : JsObject(value)
{
}

bool Subscription::IsListed()
{
  JsValuePtr func = jsEngine->Evaluate("API.isListedFilter");
  JsValueList params;
  params.push_back(shared_from_this());
  return func->Call(params)->AsBool();
}

void Subscription::AddToList()
{
  JsValuePtr func = jsEngine->Evaluate("API.addSubscriptionToList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
}

void Subscription::RemoveFromList()
{
  JsValuePtr func = jsEngine->Evaluate("API.removeSubscriptionFromList");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
}

void Subscription::UpdateFilters()
{
  JsValuePtr func = jsEngine->Evaluate("API.updateSubscription");
  JsValueList params;
  params.push_back(shared_from_this());
  func->Call(params);
}

bool Subscription::IsUpdating()
{
  JsValuePtr func = jsEngine->Evaluate("API.isSubscriptionUpdating");
  JsValueList params;
  params.push_back(shared_from_this());
  JsValuePtr result = func->Call(params);
  return result->AsBool();
}

bool Subscription::operator==(const Subscription& subscription) const
{
  return GetProperty("url", "") == subscription.GetProperty("url", "");
}

FilterEngine::FilterEngine(JsEnginePtr jsEngine) : jsEngine(jsEngine)
{
  for (int i = 0; jsSources[i] && jsSources[i + 1]; i += 2)
    jsEngine->Evaluate(jsSources[i + 1], jsSources[i]);
}

FilterPtr FilterEngine::GetFilter(const std::string& text)
{
  JsValuePtr func = jsEngine->Evaluate("API.getFilterFromText");
  JsValueList params;
  params.push_back(jsEngine->NewValue(text));
  return FilterPtr(new Filter(func->Call(params)));
}

SubscriptionPtr FilterEngine::GetSubscription(const std::string& url)
{
  JsValuePtr func = jsEngine->Evaluate("API.getSubscriptionFromUrl");
  JsValueList params;
  params.push_back(jsEngine->NewValue(url));
  return SubscriptionPtr(new Subscription(func->Call(params)));
}

std::vector<FilterPtr> FilterEngine::GetListedFilters() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getListedFilters");
  JsValueList values = func->Call()->AsList();
  std::vector<FilterPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(FilterPtr(new Filter(*it)));
  return result;
}

std::vector<SubscriptionPtr> FilterEngine::GetListedSubscriptions() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getListedSubscriptions");
  JsValueList values = func->Call()->AsList();
  std::vector<SubscriptionPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(SubscriptionPtr(new Subscription(*it)));
  return result;
}

std::vector<SubscriptionPtr> FilterEngine::FetchAvailableSubscriptions() const
{
  JsValuePtr func = jsEngine->Evaluate("API.getRecommendedSubscriptions");
  JsValueList values = func->Call()->AsList();
  std::vector<SubscriptionPtr> result;
  for (JsValueList::iterator it = values.begin(); it != values.end(); it++)
    result.push_back(SubscriptionPtr(new Subscription(*it)));
  return result;
}

AdblockPlus::FilterPtr FilterEngine::Matches(const std::string& url,
    const std::string& contentType,
    const std::string& documentUrl)
{
  JsValuePtr func = jsEngine->Evaluate("API.checkFilterMatch");
  JsValueList params;
  params.push_back(jsEngine->NewValue(url));
  params.push_back(jsEngine->NewValue(contentType));
  params.push_back(jsEngine->NewValue(documentUrl));
  JsValuePtr result = func->Call(params);
  if (!result->IsNull())
    return FilterPtr(new Filter(result));
  else
    return FilterPtr();
}

std::vector<std::string> FilterEngine::GetElementHidingSelectors(const std::string& domain) const
{
  JsValuePtr func = jsEngine->Evaluate("API.getElementHidingSelectors");
  JsValueList params;
  params.push_back(jsEngine->NewValue(domain));
  JsValueList result = func->Call(params)->AsList();
  std::vector<std::string> selectors;
  for (JsValueList::iterator it = result.begin(); it != result.end(); ++it)
    selectors.push_back((*it)->AsString());
  return selectors;
}
