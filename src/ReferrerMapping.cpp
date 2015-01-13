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

#include <AdblockPlus/ReferrerMapping.h>

using namespace AdblockPlus;

ReferrerMapping::ReferrerMapping(const int maxCachedUrls)
  : maxCachedUrls(maxCachedUrls)
{
}

void ReferrerMapping::Add(const std::string& url, const std::string& referrer)
{
  if (mapping.find(url) != mapping.end())
    cachedUrls.remove(url);
  cachedUrls.push_back(url);
  mapping[url] = referrer;

  const int urlsToPop = cachedUrls.size() - maxCachedUrls;
  for (int i = 0; i < urlsToPop; i++)
  {
    const std::string poppedUrl = cachedUrls.front();
    cachedUrls.pop_front();
    mapping.erase(poppedUrl);
  }
}

std::vector<std::string> ReferrerMapping::BuildReferrerChain(
  const std::string& url) const
{
  std::vector<std::string> referrerChain;
  referrerChain.push_back(url);
  // We need to limit the chain length to ensure we don't block indefinitely
  // if there's a referrer loop.
  const int maxChainLength = 10;
  std::map<std::string, std::string>::const_iterator currentEntry =
    mapping.find(url);
  for (int i = 0; i < maxChainLength && currentEntry != mapping.end(); i++)
  {
    const std::string& currentUrl = currentEntry->second;
    referrerChain.insert(referrerChain.begin(), currentUrl);
    currentEntry = mapping.find(currentUrl);
  }
  return referrerChain;
}
