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

#include "SubscriptionsCommand.h"

#include <iostream>
#include <sstream>

namespace
{
  typedef std::vector<AdblockPlus::Subscription> SubscriptionList;

  void ShowSubscriptionList(const SubscriptionList& subscriptions)
  {
    for (SubscriptionList::const_iterator it = subscriptions.begin(); it != subscriptions.end();
         it++)
    {
      std::cout << it->GetTitle();
      std::cout << " - " << it->GetUrl();
      std::string author = it->GetAuthor();
      if (!author.empty())
        std::cout << " - " << author;
      std::cout << std::endl;
    }
  }
}

SubscriptionsCommand::SubscriptionsCommand(AdblockPlus::IFilterEngine& filterEngine)
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
    if (url.size())
      AddSubscription(url);
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

void SubscriptionsCommand::AddSubscription(const std::string& url)
{
  auto subscription = filterEngine.GetSubscription(url);
  filterEngine.AddSubscription(subscription);
}

void SubscriptionsCommand::RemoveSubscription(const std::string& url)
{
  auto subscription = filterEngine.GetSubscription(url);
  auto listed = filterEngine.GetListedSubscriptions();
  auto iter = std::find(listed.begin(), listed.end(), subscription);
  if (iter == listed.end())
  {
    std::cout << "No subscription with URL '" << url << "'" << std::endl;
    return;
  }
  filterEngine.RemoveSubscription(subscription);
}

void SubscriptionsCommand::UpdateSubscriptions()
{
  SubscriptionList subscriptions = filterEngine.GetListedSubscriptions();
  for (SubscriptionList::iterator it = subscriptions.begin(); it != subscriptions.end(); it++)
    it->UpdateFilters();
}

void SubscriptionsCommand::FetchSubscriptions()
{
  ShowSubscriptionList(filterEngine.FetchAvailableSubscriptions());
}
