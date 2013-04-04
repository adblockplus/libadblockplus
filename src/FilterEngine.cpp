#include <AdblockPlus.h>

using namespace AdblockPlus;

#if !FILTER_ENGINE_STUBS
extern const char* jsSources[];
#endif

#if FILTER_ENGINE_STUBS
Subscription::Subscription(FilterEngine& filterEngine, const std::string& url)
    : filterEngine(filterEngine)
{
  properties["url"] = url;
}
#else
Subscription::Subscription()
{
}
#endif

const std::string Subscription::GetProperty(const std::string& name) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string,std::string>::const_iterator it = properties.find(name);
  if (it == properties.end())
    return "";
  else
    return it->second;
#endif
}

int Subscription::GetIntProperty(const std::string& name) const
{
#if FILTER_ENGINE_STUBS
  std::map<std::string,int>::const_iterator it = intProperties.find(name);
  if (it == intProperties.end())
    return 0;
  else
    return it->second;
#endif
}

void Subscription::SetProperty(const std::string& name, const std::string& value)
{
#if FILTER_ENGINE_STUBS
  properties[name] = value;
#endif
}

void Subscription::SetIntProperty(const std::string& name, int value)
{
#if FILTER_ENGINE_STUBS
  intProperties[name] = value;
#endif
}

bool Subscription::IsListed() const
{
#if FILTER_ENGINE_STUBS
  for (std::vector<Subscription*>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end(); ++it)
  {
    if (*it == this)
      return true;
  }
  return false;
#endif
}

void Subscription::AddToList()
{
#if FILTER_ENGINE_STUBS
  if (!IsListed())
    filterEngine.listedSubscriptions.push_back(this);
#endif
}

void Subscription::RemoveFromList()
{
  for (std::vector<Subscription*>::iterator it = filterEngine.listedSubscriptions.begin();
       it != filterEngine.listedSubscriptions.end();)
  {
    if (*it == this)
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

Subscription& FilterEngine::GetSubscription(const std::string& url)
{
#if FILTER_ENGINE_STUBS
  std::map<std::string,Subscription*>::const_iterator it = knownSubscriptions.find(url);
  if (it != knownSubscriptions.end())
    return *it->second;

  Subscription* result = new Subscription(*this, url);
  knownSubscriptions[url] = result;
  return *result;
#endif
}

const std::vector<Subscription*>& FilterEngine::GetListedSubscriptions() const
{
#if FILTER_ENGINE_STUBS
  return listedSubscriptions;
#endif
}

void FilterEngine::FetchAvailableSubscriptions(SubscriptionsCallback callback)
{
#if FILTER_ENGINE_STUBS
  std::vector<Subscription*> availableSubscriptions;

  Subscription& subscription1 = GetSubscription("https://easylist-downloads.adblockplus.org/easylist.txt");
  subscription1.SetProperty("title", "EasyList");
  availableSubscriptions.push_back(&subscription1);

  Subscription& subscription2 = GetSubscription("https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt");
  subscription2.SetProperty("title", "EasyList Germany+EasyList");
  availableSubscriptions.push_back(&subscription2);

  callback(availableSubscriptions);
#endif
}

bool FilterEngine::Matches(const std::string& url,
                           const std::string& contentType,
                           const std::string& documentUrl) const
{
#if FILTER_ENGINE_STUBS
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  return url.find("adbanner.gif") != std::string::npos;
#endif
}

std::vector<std::string> FilterEngine::GetElementHidingRules() const
{
#if FILTER_ENGINE_STUBS
  std::vector<std::string> hidingRules;
  hidingRules.push_back("###ad");
  hidingRules.push_back("##.ad");
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  hidingRules.push_back("##.ad_300x250");
  return hidingRules;
#endif
}
