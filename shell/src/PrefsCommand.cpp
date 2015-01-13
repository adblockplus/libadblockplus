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

#include "PrefsCommand.h"

PrefsCommand::PrefsCommand(AdblockPlus::FilterEngine& filterEngine)
  : Command("prefs"), filterEngine(filterEngine)
{
}

void PrefsCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentStream(arguments);
  std::string action;
  argumentStream >> action;
  if (!action.size())
  {
    ShowUsage();
    return;
  }

  if (action == "show")
  {
    std::string pref;
    argumentStream >> pref;

    AdblockPlus::JsValuePtr value = filterEngine.GetPref(pref);
    if (value->IsUndefined())
      std::cout << "No such preference" << std::endl;
    else
    {
      if (value->IsString())
        std::cout << "(string) ";
      else if (value->IsNumber())
        std::cout << "(number) ";
      else if (value->IsBool())
        std::cout << "(bool) ";
      else
        std::cout << "(unknown type) ";
      std::cout << value->AsString() << std::endl;
    }
  }
  else if (action == "set")
  {
    std::string pref;
    argumentStream >> pref;

    AdblockPlus::JsValuePtr current = filterEngine.GetPref(pref);
    if (current->IsUndefined())
      std::cout << "No such preference" << std::endl;
    else if (current->IsString())
    {
      std::string value;
      std::getline(argumentStream, value);
      filterEngine.SetPref(pref, filterEngine.GetJsEngine()->NewValue(value));
    }
    else if (current->IsNumber())
    {
      int64_t value;
      argumentStream >> value;
      filterEngine.SetPref(pref, filterEngine.GetJsEngine()->NewValue(value));
    }
    else if (current->IsBool())
    {
      bool value;
      argumentStream >> value;
      filterEngine.SetPref(pref, filterEngine.GetJsEngine()->NewValue(value));
    }
    else
      std::cout << "Cannot set a preference of unknown type" << std::endl;
  }
  else
    throw NoSuchCommandError(name + " " + action);
}

std::string PrefsCommand::GetDescription() const
{
  return "Get and set preferences";
}

std::string PrefsCommand::GetUsage() const
{
  return name + " [show PREF|set PREF VALUE]";
}
