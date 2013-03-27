#include <iostream>
#include <sstream>

#include "SubscriptionsCommand.h"

namespace
{
  typedef std::vector<AdblockPlus::Subscription> SubscriptionList;

  void ShowSubscriptionList(const SubscriptionList& subscriptions)
  {
    for (SubscriptionList::const_iterator it = subscriptions.begin();
         it != subscriptions.end(); it++)
      std::cout << it->title << " - " << it->url << std::endl;
  }
}

SubscriptionsCommand::SubscriptionsCommand(
  AdblockPlus::FilterEngine& filterEngine)
  : Command("subscriptions"), filterEngine(filterEngine)
{
}

void SubscriptionsCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentStream(arguments);
  std::string action;
  argumentStream >> action;
  if (!action.size())
  {
    ShowSubscriptions();
    return;
  }

  if (action == "add")
  {
    std::string url;
    argumentStream >> url;
    std::string title;
    std::getline(argumentStream, title);
    if (url.size() || title.size())
      AddSubscription(url, title);
    else
      ShowUsage();
  }
  else if (action == "remove")
  {
    std::string url;
    argumentStream >> url;
    if (url.size())
      RemoveSubscription(url);
    else
      ShowUsage();
  }
  else if (action == "update")
    UpdateSubscriptions();
  else if (action == "fetch")
    FetchSubscriptions();
  else
    throw NoSuchCommandError(name + " " + action);
}

std::string SubscriptionsCommand::GetDescription() const
{
  return "List and manage subscriptions";
}

std::string SubscriptionsCommand::GetUsage() const
{
  return name + " [add URL TITLE|remove URL|update|fetch]";
}

void SubscriptionsCommand::ShowSubscriptions()
{
  ShowSubscriptionList(filterEngine.GetSubscriptions());
}

void SubscriptionsCommand::AddSubscription(const std::string& url,
                                           const std::string& title)
{
  filterEngine.AddSubscription(AdblockPlus::Subscription(url, title));
}

void SubscriptionsCommand::RemoveSubscription(const std::string& url)
{
  const AdblockPlus::Subscription* const subscription =
    filterEngine.FindSubscription(url);
  if (!subscription)
  {
    std::cout << "No subscription with URL '" << url << "'" << std::endl;
    return;
  }
  filterEngine.RemoveSubscription(*subscription);
}

void SubscriptionsCommand::UpdateSubscriptions()
{
  const SubscriptionList& subscriptions = filterEngine.GetSubscriptions();
  for (SubscriptionList::const_iterator it = subscriptions.begin();
       it != subscriptions.end(); it++)
    filterEngine.UpdateSubscriptionFilters(*it);
}

void SubscriptionsCommand::FetchSubscriptions()
{
  ShowSubscriptionList(filterEngine.FetchAvailableSubscriptions());
}
