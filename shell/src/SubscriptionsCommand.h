#ifndef SUBSCRIPTIONS_COMMAND_H
#define SUBSCRIPTIONS_COMMAND_H

#include <string>

#include "Command.h"

class SubscriptionsCommand : public Command
{
public:
  SubscriptionsCommand();
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  void ShowSubscriptions();
  void AddSubscription(const std::string& url);
  void RemoveSubscription(const std::string& url);
  void UpdateSubscriptions();
};

#endif
