#include <iostream>

#include "Command.h"

Command::Command(const std::string& name) : name(name)
{
}

Command::~Command()
{
}

void Command::ShowUsage() const
{
  std::cout << "Usage: " << GetUsage() << std::endl;
}

NoSuchCommandError::NoSuchCommandError(const std::string& commandName)
  : std::runtime_error("No such command: " + commandName)
{
}
