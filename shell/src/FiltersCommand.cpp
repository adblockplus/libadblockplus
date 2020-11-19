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

#include "FiltersCommand.h"

#include <iostream>
#include <sstream>

namespace
{
  typedef std::vector<AdblockPlus::Filter> FilterList;

  void ShowFilterList(const FilterList& filters)
  {
    for (FilterList::const_iterator it = filters.begin(); it != filters.end(); it++)
    {
      std::string type;
      switch (it->GetType())
      {
      case AdblockPlus::IFilterImplementation::TYPE_BLOCKING:
        type = "blocking";
        break;
      case AdblockPlus::IFilterImplementation::TYPE_EXCEPTION:
        type = "exception";
        break;
      case AdblockPlus::IFilterImplementation::TYPE_ELEMHIDE:
        type = "elemhide";
        break;
      case AdblockPlus::IFilterImplementation::TYPE_ELEMHIDE_EXCEPTION:
        type = "elemhideexception";
        break;
      case AdblockPlus::IFilterImplementation::TYPE_COMMENT:
        type = "comment";
        break;
      case AdblockPlus::IFilterImplementation::TYPE_INVALID:
        type = "invalid";
        break;
      default:
        type = "(unknown type)";
        break;
      }
      std::cout << it->GetRaw() << " - " << type << std::endl;
    }
  }
}

FiltersCommand::FiltersCommand(AdblockPlus::IFilterEngine& filterEngine)
    : Command("filters"), filterEngine(filterEngine)
{
}

void FiltersCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentStream(arguments);
  std::string action;
  argumentStream >> action;
  if (!action.size())
  {
    ShowFilters();
    return;
  }

  if (action == "add")
  {
    std::string text;
    std::getline(argumentStream, text);
    if (text.size())
      AddFilter(text);
    else
      ShowUsage();
  }
  else if (action == "remove")
  {
    std::string text;
    std::getline(argumentStream, text);
    if (text.size())
      RemoveFilter(text);
    else
      ShowUsage();
  }
  else
    throw NoSuchCommandError(name + " " + action);
}

std::string FiltersCommand::GetDescription() const
{
  return "List and manage custom filters";
}

std::string FiltersCommand::GetUsage() const
{
  return name + " [add FILTER|remove FILTER|<empty> (to list)]";
}

void FiltersCommand::ShowFilters()
{
  ShowFilterList(filterEngine.GetListedFilters());
}

void FiltersCommand::AddFilter(const std::string& text)
{
  auto filter = filterEngine.GetFilter(text);
  filterEngine.AddFilter(filter);
}

void FiltersCommand::RemoveFilter(const std::string& text)
{
  auto filter = filterEngine.GetFilter(text);
  auto listed = filterEngine.GetListedFilters();
  auto iter = std::find(listed.begin(), listed.end(), filter);
  if (iter == listed.end())
  {
    std::cout << "No such filter '" << text << "'" << std::endl;
    return;
  }
  filterEngine.RemoveFilter(filter);
}
