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
