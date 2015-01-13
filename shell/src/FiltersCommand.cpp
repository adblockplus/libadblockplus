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

#include "FiltersCommand.h"

namespace
{
  typedef std::vector<AdblockPlus::FilterPtr> FilterList;

  void ShowFilterList(const FilterList& filters)
  {
    for (FilterList::const_iterator it = filters.begin();
         it != filters.end(); it++)
    {
      std::string type;
      switch ((*it)->GetType())
      {
        case AdblockPlus::Filter::TYPE_BLOCKING:
          type = "blocking";
          break;
        case AdblockPlus::Filter::TYPE_EXCEPTION:
          type = "exception";
          break;
        case AdblockPlus::Filter::TYPE_ELEMHIDE:
          type = "elemhide";
          break;
        case AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION:
          type = "elemhideexception";
          break;
        case AdblockPlus::Filter::TYPE_COMMENT:
          type = "comment";
          break;
        case AdblockPlus::Filter::TYPE_INVALID:
          type = "invalid";
          break;
        default:
          type = "(unknown type)";
          break;
      }
      std::cout << (*it)->GetProperty("text")->AsString() << " - " <<
          type << std::endl;
    }
  }
}

FiltersCommand::FiltersCommand(AdblockPlus::FilterEngine& filterEngine)
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
  return name + " [add FILTER|remove FILTER]";
}

void FiltersCommand::ShowFilters()
{
  ShowFilterList(filterEngine.GetListedFilters());
}

void FiltersCommand::AddFilter(const std::string& text)
{
  AdblockPlus::FilterPtr filter = filterEngine.GetFilter(text);
  filter->AddToList();
}

void FiltersCommand::RemoveFilter(const std::string& text)
{
  AdblockPlus::FilterPtr filter = filterEngine.GetFilter(text);
  if (!filter->IsListed())
  {
    std::cout << "No such filter '" << text << "'" << std::endl;
    return;
  }
  filter->RemoveFromList();
}
