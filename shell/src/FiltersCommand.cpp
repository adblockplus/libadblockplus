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
      std::cout << (*it)->GetProperty("text", "(no text)") << " - " <<
          (*it)->GetProperty("type", "(no type)") << std::endl;
    }
  }
}

FiltersCommand::FiltersCommand(
  AdblockPlus::FilterEngine& filterEngine)
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
  AdblockPlus::Filter& filter = filterEngine.GetFilter(text);
  filter.AddToList();
}

void FiltersCommand::RemoveFilter(const std::string& text)
{
  AdblockPlus::Filter& filter = filterEngine.GetFilter(text);
  if (!filter.IsListed())
  {
    std::cout << "No such filter '" << text << "'" << std::endl;
    return;
  }
  filter.RemoveFromList();
}
