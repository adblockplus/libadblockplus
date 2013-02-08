#ifndef MATCHES_COMMAND_H
#define MATCHES_COMMAND_H

#include "Command.h"

struct MatchesCommand : public Command
{
  MatchesCommand();
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;
};

#endif
