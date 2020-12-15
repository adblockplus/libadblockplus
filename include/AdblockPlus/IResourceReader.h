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

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace AdblockPlus
{
  class IPreloadedFilterResponse
  {
  public:
    virtual ~IPreloadedFilterResponse() = default;

    /**
     * true if resource associated with url was found
     */
    virtual bool exists() const = 0;

    /**
     * Preloaded filter list content if found. Format should be same as it is downloaded from
     * server. Following special comments will be processed: Expires, Title, Homepage. All other
     * are ignored. If Expires is not set, invalid or outdated, subscription will be scheduled for
     * update once engine is enabled and ready. Expected encoding is UTF-8.
     */
    virtual const char* content() const = 0;

    virtual size_t size() const = 0;
  };

  class StringPreloadedFilterResponse : public IPreloadedFilterResponse
  {
  public:
    /**
     * exists will mean !data.empty()
     */
    StringPreloadedFilterResponse(std::string data = std::string());
    StringPreloadedFilterResponse(const StringPreloadedFilterResponse&) = delete;
    StringPreloadedFilterResponse& operator=(const StringPreloadedFilterResponse&) = delete;

    bool exists() const override;
    const char* content() const override;
    virtual size_t size() const override;

  private:
    std::string data;
  };

  /**
   * Required to get an access to preloaded subscriptions.
   */
  class IResourceReader
  {
  public:
    virtual ~IResourceReader() = default;

    using ReadCallback = std::function<void(std::unique_ptr<IPreloadedFilterResponse>)>;

    /**
     * Gives possibility for the embedder to provide filter lists for subscription URL before
     * network request is done.
     * @param url for subscription.
     * @param doneCallback The function to be called on completion. @see PreloadedFilterResponse
     */
    virtual void ReadPreloadedFilterList(const std::string& url,
                                         const ReadCallback& doneCallback) const = 0;
  };
}
