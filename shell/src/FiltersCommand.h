#ifndef FILTERS_COMMAND_H
#define FILTERS_COMMAND_H

#include <AdblockPlus.h>
#include <string>

#include "Command.h"

class FiltersCommand : public Command
{
public:
  explicit FiltersCommand(AdblockPlus::FilterEngine& filterEngine);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  AdblockPlus::FilterEngine& filterEngine;

  void ShowFilters();
  void AddFilter(const std::string& text);
  void RemoveFilter(const std::string& text);
};

#endif
