#include <iostream>
#include <sstream>

#include "SubscriptionsCommand.h"

namespace
{
  typedef std::vector<AdblockPlus::SubscriptionPtr> SubscriptionList;

  void ShowSubscriptionList(const SubscriptionList& subscriptions)
  {
    for (SubscriptionList::const_iterator it = subscriptions.begin();
         it != subscriptions.end(); it++)
    {
      std::cout << (*it)->GetProperty("title", "(no title)") << " - " << (*it)->GetProperty("url", "");
      if ((*it)->GetProperty("author", "") != "")
        std::cout << " - " << (*it)->GetProperty("author", "");
      if ((*it)->GetProperty("specialization", "") != "")
        std::cout << " - " << (*it)->GetProperty("specialization", "");
      std::cout << std::endl;
    }
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
    if (url.size())
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
  return name + " [add URL [TITLE]|remove URL|update|fetch]";
}

void SubscriptionsCommand::ShowSubscriptions()
{
  ShowSubscriptionList(filterEngine.GetListedSubscriptions());
}

void SubscriptionsCommand::AddSubscription(const std::string& url,
                                           const std::string& title)
{
  AdblockPlus::SubscriptionPtr subscription = filterEngine.GetSubscription(url);
  if (title.size())
    subscription->SetProperty("title", title);
  subscription->AddToList();
}

void SubscriptionsCommand::RemoveSubscription(const std::string& url)
{
  AdblockPlus::SubscriptionPtr subscription = filterEngine.GetSubscription(url);
  if (!subscription->IsListed())
  {
    std::cout << "No subscription with URL '" << url << "'" << std::endl;
    return;
  }
  subscription->RemoveFromList();
}

void SubscriptionsCommand::UpdateSubscriptions()
{
  const SubscriptionList& subscriptions = filterEngine.GetListedSubscriptions();
  for (SubscriptionList::const_iterator it = subscriptions.begin();
       it != subscriptions.end(); it++)
    (*it)->UpdateFilters();
}

void SubscriptionsCommand::FetchSubscriptions()
{
  ShowSubscriptionList(filterEngine.FetchAvailableSubscriptions());
}
