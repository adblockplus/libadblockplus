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

#ifndef SUBSCRIPTIONS_COMMAND_H
#define SUBSCRIPTIONS_COMMAND_H

#include <AdblockPlus.h>
#include <string>

#include "Command.h"

class SubscriptionsCommand : public Command
{
public:
  explicit SubscriptionsCommand(AdblockPlus::FilterEngine& filterEngine);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  AdblockPlus::FilterEngine& filterEngine;

  void ShowSubscriptions();
  void AddSubscription(const std::string& url, const std::string& title);
  void RemoveSubscription(const std::string& url);
  void UpdateSubscriptions();
  void FetchSubscriptions();
};

#endif
