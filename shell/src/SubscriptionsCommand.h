#ifndef SUBSCRIPTIONS_COMMAND_H
#define SUBSCRIPTIONS_COMMAND_H

#include <AdblockPlus.h>
#include <string>

#include "Command.h"

class SubscriptionsCommand : public Command
{
public:
  SubscriptionsCommand(AdblockPlus::JsEngine& jsEngine);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  AdblockPlus::JsEngine& jsEngine;

  void ShowSubscriptions();
  void AddSubscription(const std::string& url);
  void RemoveSubscription(const std::string& url);
  void UpdateSubscriptions();
};

#endif
