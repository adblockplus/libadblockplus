/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#ifndef ADBLOCK_PLUS_REFERRER_MAPPING_H
#define ADBLOCK_PLUS_REFERRER_MAPPING_H

#include <list>
#include <map>
#include <string>
#include <vector>

namespace AdblockPlus
{
  /**
   * Stores a mapping between URLs and their referrers.
   * This can be used to build a chain of referrers for any URL
   * (see `BuildReferrerChain()`), which approximates the frame structure, see
   * FilterEngine::Matches().
   */
  class ReferrerMapping
  {
  public:
    /**
     * Constructor.
     * @param maxCachedUrls Number of URL mappings to store. The higher the
     *        better - clients typically cache requests, and a single cached
     *        request will break the referrer chain.
     */
    ReferrerMapping(const int maxCachedUrls = 5000);

    /**
     * Records the refferer for a URL.
     * @param url Request URL.
     * @param referrer Request referrer.
     */
    void Add(const std::string& url, const std::string& referrer);

    /**
     * Builds a chain of referrers for the supplied URL.
     * This should reconstruct a document's parent frame URLs.
     * @param url URL to build the chain for.
     * @return List of URLs, starting with `url`.
     */
    std::vector<std::string> BuildReferrerChain(const std::string& url) const;

  private:
    const int maxCachedUrls;
    std::map<std::string, std::string> mapping;
    std::list<std::string> cachedUrls;
  };
}

#endif
