/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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
      std::cout << (*it)->GetProperty("title")->AsString();
      std::cout << " - " << (*it)->GetProperty("url")->AsString();
      if (!(*it)->GetProperty("author")->IsUndefined())
        std::cout << " - " << (*it)->GetProperty("author")->AsString();
      if (!(*it)->GetProperty("specialization")->IsUndefined())
        std::cout << " - " << (*it)->GetProperty("specialization")->AsString();
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
