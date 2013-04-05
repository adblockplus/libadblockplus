#include <iostream>
#include <sstream>

#include "MatchesCommand.h"

MatchesCommand::MatchesCommand(AdblockPlus::FilterEngine& filterEngine)
  : Command("matches"), filterEngine(filterEngine)
{
}

void MatchesCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentStream(arguments);
  std::string url;
  argumentStream >> url;
  std::string contentType;
  argumentStream >> contentType;
  std::string documentUrl;
  argumentStream >> documentUrl;
  if (!url.size() || !contentType.size() || !documentUrl.size())
  {
    ShowUsage();
    return;
  }

  AdblockPlus::Filter* match = filterEngine.Matches(url, contentType, documentUrl);
  if (!match)
    std::cout << "No match" << std::endl;
  else if (match->GetProperty("type", "") == "exception")
    std::cout << "Whitelisted" << std::endl;
  else
    std::cout << "Blocked" << std::endl;
}

std::string MatchesCommand::GetDescription() const
{
  return "Returns the first filter that matches the supplied URL";
}

std::string MatchesCommand::GetUsage() const
{
  return name + " URL CONTENT_TYPE DOCUMENT_URL";
}
