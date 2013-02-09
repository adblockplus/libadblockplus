#ifndef MATCHES_COMMAND_H
#define MATCHES_COMMAND_H

#include "Command.h"

class MatchesCommand : public Command
{
public:
  MatchesCommand();
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;
};

#endif
