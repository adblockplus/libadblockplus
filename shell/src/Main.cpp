/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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

#include "../src/DefaultPlatform.h"
#include "../src/JsEngine.h"
#include "FiltersCommand.h"
#include "GcCommand.h"
#include "HelpCommand.h"
#include "MatchesCommand.h"
#include "PrefsCommand.h"
#include "SubscriptionsCommand.h"

namespace
{
  void Add(CommandMap& commands, std::unique_ptr<Command> command)
  {
    commands[command->name].swap(command);
  }

  bool ReadCommandLine(std::string& commandLine)
  {
    std::cout << "> ";
    const bool success = std::getline(std::cin, commandLine).good();
    if (!success)
      std::cout << std::endl;
    return success;
  }

  void ParseCommandLine(const std::string& commandLine, std::string& name, std::string& arguments)
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
    appInfo.application = "standalone";
    appInfo.applicationVersion = "1.0";
    appInfo.locale = "en-US";

    auto platform = AdblockPlus::PlatformFactory::CreatePlatform();
    platform->SetUp(appInfo);
    AdblockPlus::JsEngine& jsEngine =
        static_cast<AdblockPlus::DefaultPlatform*>(platform.get())->GetJsEngine();
    auto& filterEngine = platform->GetFilterEngine();

    CommandMap commands;
    Add(commands, std::make_unique<GcCommand>(jsEngine));
    Add(commands, std::make_unique<HelpCommand>(commands));
    Add(commands, std::make_unique<FiltersCommand>(filterEngine));
    Add(commands, std::make_unique<SubscriptionsCommand>(filterEngine));
    Add(commands, std::make_unique<MatchesCommand>(filterEngine));
    Add(commands, std::make_unique<PrefsCommand>(filterEngine, jsEngine));

    std::string commandLine;
    while (ReadCommandLine(commandLine))
    {
      if (commandLine == "exit" || commandLine == "q")
      {
        break;
      }
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
