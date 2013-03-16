#ifndef HELP_COMMAND_H
#define HELP_COMMAND_H

#include "Command.h"

class HelpCommand : public Command
{
public:
  explicit HelpCommand(const CommandMap& commands);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  const CommandMap& commands;

  void ShowCommandHelp(const std::string& commandName) const;
  void ShowCommandList() const;
};

#endif
