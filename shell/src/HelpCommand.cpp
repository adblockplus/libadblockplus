/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

#include <iostream>
#include <sstream>

#include "HelpCommand.h"

HelpCommand::HelpCommand(const CommandMap& commands)
  : Command("help"), commands(commands)
{
}

void HelpCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentsStream(arguments);
  std::string commandName;
  argumentsStream >> commandName;
  if (commandName.size())
    ShowCommandHelp(commandName);
  else
    ShowCommandList();
}

std::string HelpCommand::GetDescription() const
{
  return "Show a list of commands, or the usage of a particular command";
}

std::string HelpCommand::GetUsage() const
{
  return name + " [COMMAND]";
}

void HelpCommand::ShowCommandHelp(const std::string& commandName) const
{
  const CommandMap::const_iterator it = commands.find(commandName);
  if (it == commands.end())
    throw NoSuchCommandError(commandName);

  const Command& command = *it->second;
  std::cout << command.GetDescription() << std::endl
            << "Usage: " << command.GetUsage() << std::endl;
}

void HelpCommand::ShowCommandList() const
{
  for (CommandMap::const_iterator it = commands.begin();
       it != commands.end(); it++)
  {
    const Command& command = *it->second;
    std::cout << command.name << " - " << command.GetDescription() << std::endl;
  }
}
