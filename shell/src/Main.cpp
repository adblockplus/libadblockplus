/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    appInfo.name = "abpshell";
    appInfo.platform = "standalone";
    appInfo.locale = "en-US";
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
