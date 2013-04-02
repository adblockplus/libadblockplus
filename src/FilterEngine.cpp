#include <AdblockPlus.h>

using namespace AdblockPlus;

Subscription::Subscription(const std::string& url, const std::string& title)
  : url(url), title(title)
{
}

FilterEngine::FilterEngine(JsEngine& jsEngine) : jsEngine(jsEngine)
{
  // TODO: Load ABP:
  // jsEngine.Load("adblockplus_compat.js");
  // jsEngine.Load("adblockplus.js");
}

void FilterEngine::AddSubscription(Subscription subscription)
{
  subscriptions.push_back(subscription);
}

void FilterEngine::RemoveSubscription(const Subscription& subscription)
{
  for (std::vector<Subscription>::iterator it = subscriptions.begin();
       it != subscriptions.end();)
    if (it->url == subscription.url)
      it = subscriptions.erase(it);
    else
      it++;
}

const Subscription* FilterEngine::FindSubscription(const std::string& url) const
{
  for (std::vector<Subscription>::const_iterator it = subscriptions.begin();
       it != subscriptions.end(); it++)
    if (it->url == url)
      return &(*it);
  return 0;
}

const std::vector<Subscription>& FilterEngine::GetSubscriptions() const
{
  return subscriptions;
}

void FilterEngine::UpdateSubscriptionFilters(const Subscription& subscription)
{
}

std::vector<Subscription> FilterEngine::FetchAvailableSubscriptions()
{
  std::vector<Subscription> availableSubscriptions;
  availableSubscriptions.push_back(Subscription("https://easylist-downloads.adblockplus.org/easylist.txt", "EasyList"));
  availableSubscriptions.push_back(Subscription("https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt", "EasyList Germany+EasyList"));
  return availableSubscriptions;
}

bool FilterEngine::Matches(const std::string& url,
                           const std::string& contentType) const
{
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  return url.find("adbanner.gif") != std::string::npos;
//  return subscriptions.size() && url.length() % 2;
}

std::vector<std::string> FilterEngine::GetElementHidingRules() const
{
  std::vector<std::string> hidingRules;
  hidingRules.push_back("###ad");
  hidingRules.push_back("##.ad");
  //For test on http://simple-adblock.com/faq/testing-your-adblocker/
  hidingRules.push_back("##.ad_300x250");
  return hidingRules;
}
