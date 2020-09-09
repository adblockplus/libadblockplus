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

#include "ElementUtils.h"
#include "Utils.h"

using namespace AdblockPlus;

namespace detail
{

  void AppendNonEmpty(std::vector<std::string>& urls, const std::string& value)
  {
    std::string trimmed = Utils::TrimString(value);

    if (!trimmed.empty())
      urls.push_back(std::move(trimmed));
  }

  void GetURLsFromGenericElement(const IElement* element, std::vector<std::string>& urls)
  {
    AppendNonEmpty(urls, element->GetAttribute("src"));

    for (const auto& cur : Utils::SplitString(element->GetAttribute("srcset"), ','))
      AppendNonEmpty(urls, cur);
  }

  void GetURLsFromObjectElement(const IElement* element, std::vector<std::string>& urls)
  {
    std::string data = Utils::TrimString(element->GetAttribute("data"));

    if (!data.empty())
    {
      AppendNonEmpty(urls, data);
      return;
    }

    for (auto cur : element->GetChildren())
    {
      if (cur->GetLocalName() != "param")
        continue;

      std::string chname = cur->GetAttribute("name");

      if (chname == "movie" || chname == "source" || chname == "src" || chname == "FileName")
        AppendNonEmpty(urls, cur->GetAttribute("value"));
    }
  }

  void GetURLsFromMediaElement(const IElement* element, std::vector<std::string>& urls)
  {
    GetURLsFromGenericElement(element, urls);
    AppendNonEmpty(urls, element->GetAttribute("poster"));

    for (auto cur : element->GetChildren())
    {
      std::string name = cur->GetLocalName();

      if (name == "source" || name == "track")
        GetURLsFromGenericElement(cur, urls);
    }
  }

} // namespace detail

std::vector<std::string> Utils::GetAssociatedUrls(const IElement* element)
{
  std::string id = element->GetLocalName();
  std::vector<std::string> urls;

  if (id == "object")
    detail::GetURLsFromObjectElement(element, urls);
  else if (id == "video" || id == "audio" || id == "picture")
    detail::GetURLsFromMediaElement(element, urls);
  else
    detail::GetURLsFromGenericElement(element, urls);

  return urls;
}
