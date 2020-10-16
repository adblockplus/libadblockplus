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

#include "MatchesCommand.h"

#include <iostream>
#include <sstream>

MatchesCommand::MatchesCommand(AdblockPlus::IFilterEngine& filterEngine)
    : Command("matches"), filterEngine(filterEngine)
{
}

void MatchesCommand::operator()(const std::string& arguments)
{
  std::istringstream argumentStream(arguments);
  std::string url;
  argumentStream >> url;
  std::string contentTypeStr;
  argumentStream >> contentTypeStr;
  std::string documentUrl;
  argumentStream >> documentUrl;
  std::string siteKey;
  argumentStream >> siteKey;
  AdblockPlus::IFilterEngine::ContentType contentType;
  try
  {
    contentType = AdblockPlus::IFilterEngine::StringToContentType(contentTypeStr);
  }
  catch (std::invalid_argument& e)
  {
    contentTypeStr.clear();
  }
  if (!url.size() || !contentTypeStr.size() || !documentUrl.size())
  {
    ShowUsage();
    return;
  }

  AdblockPlus::Filter match = filterEngine.Matches(url, contentType, documentUrl, siteKey);
  if (!match.IsValid())
    return;
  if (match.GetType() == AdblockPlus::IFilterImplementation::TYPE_EXCEPTION)
    std::cout << "Whitelisted by " << match.GetRaw() << std::endl;
  else
    std::cout << "Blocked by " << match.GetRaw() << std::endl;
}

std::string MatchesCommand::GetDescription() const
{
  return "Returns the first filter that matches the supplied URL";
}

std::string MatchesCommand::GetUsage() const
{
  return name + " URL CONTENT_TYPE DOCUMENT_URL [SITEKEY]";
}
