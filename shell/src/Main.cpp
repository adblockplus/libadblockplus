#include <iostream>
#include <sstream>

#include "HelpCommand.h"

// TODO: Implement commands for subscription handling and URL matching

namespace
{
  void Add(CommandMap& commands, Command* command)
  {
    commands[command->name] = command;
  }

  bool ReadCommandLine(std::string& commandLine)
  {
    std::cout << "> ";
    bool success = std::getline(std::cin, commandLine);
    if (!success)
      std::cout << std::endl;
    return success;
  }

  void ParseCommandLine(const std::string& commandLine, std::string& name,
                        std::string& arguments)
  {
    std::istringstream lineStream(commandLine);
    lineStream >> name;
    std::getline(lineStream, arguments);
  }
}

int main()
{
  CommandMap commands;
  Add(commands, new HelpCommand(commands));
  std::string commandLine;
  while (ReadCommandLine(commandLine))
  {
    std::string commandName;
    std::string arguments;
    ParseCommandLine(commandLine, commandName, arguments);
    CommandMap::const_iterator it = commands.find(commandName);
    try
    {
      if (it != commands.end())
        (*it->second)(arguments);
      else
        throw NoSuchCommandError(commandName);
    }
    catch (NoSuchCommandError error)
    {
      std::cout << error.what() << std::endl;
    }
  }
  return 0;
}
