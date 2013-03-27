#ifndef MATCHES_COMMAND_H
#define MATCHES_COMMAND_H

#include <AdblockPlus.h>

#include "Command.h"

class MatchesCommand : public Command
{
public:
  MatchesCommand(AdblockPlus::FilterEngine& filterEngine);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  AdblockPlus::FilterEngine& filterEngine;
};

#endif
