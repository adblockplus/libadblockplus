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

#include <AdblockPlus/IExecutor.h>
#include <AdblockPlus/IWebRequest.h>

namespace AdblockPlus
{

  /**
   * Synchronous web request interface, used for requests using OS libraries.
   */
  struct IWebRequestSync
  {
    virtual ~IWebRequestSync() = default;
    virtual ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const = 0;
  };

  typedef std::unique_ptr<IWebRequestSync> WebRequestSyncPtr;

  class DefaultWebRequestSync : public IWebRequestSync
  {
  public:
    ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const override;
  };

  /**
   * Asynchronous web request, implemented as a wrapper of a synchronous one.
   */
  class DefaultWebRequest : public IWebRequest
  {
  public:
    DefaultWebRequest(IExecutor& executor, WebRequestSyncPtr syncImpl);
    ~DefaultWebRequest();

    void GET(const std::string& url,
             const HeaderList& requestHeaders,
             const GetCallback& getCallback) override;

  private:
    IExecutor& executor;
    WebRequestSyncPtr syncImpl;
  };
}
