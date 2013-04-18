#include <AdblockPlus.h>
#include <iostream>
#include <sstream>

#include "GcCommand.h"
#include "HelpCommand.h"
#include "FiltersCommand.h"
#include "SubscriptionsCommand.h"
#include "MatchesCommand.h"

namespace
{
  void Add(CommandMap& commands, Command* command)
  {
    commands[command->name] = command;
  }

  bool ReadCommandLine(std::string& commandLine)
  {
    std::cout << "> ";
    const bool success = std::getline(std::cin, commandLine);
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
  try
  {
    AdblockPlus::AppInfo appInfo;
    appInfo.version = "1.0";
    appInfo.name = "Adblock Plus Shell";
    AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New(appInfo));
    AdblockPlus::FilterEngine filterEngine(jsEngine);

    CommandMap commands;
    Add(commands, new GcCommand(jsEngine));
    Add(commands, new HelpCommand(commands));
    Add(commands, new FiltersCommand(filterEngine));
    Add(commands, new SubscriptionsCommand(filterEngine));
    Add(commands, new MatchesCommand(filterEngine));

    std::string commandLine;
    while (ReadCommandLine(commandLine))
    {
      std::string commandName;
      std::string arguments;
      ParseCommandLine(commandLine, commandName, arguments);
      const CommandMap::const_iterator it = commands.find(commandName);
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
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
