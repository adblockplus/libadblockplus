#include "MatchesCommand.h"

MatchesCommand::MatchesCommand() : Command("matches")
{
}

void MatchesCommand::operator()(const std::string& arguments)
{
}

std::string MatchesCommand::GetDescription() const
{
  return "Returns the first filter that matches the supplied URL";
}

std::string MatchesCommand::GetUsage() const
{
  return name + " URL";
}
