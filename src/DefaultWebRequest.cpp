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

#include "DefaultWebRequest.h"

#include <thread>

using namespace AdblockPlus;

AdblockPlus::ServerResponse
AdblockPlus::DefaultWebRequestSync::GET(const std::string& url,
                                        const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = IWebRequest::NS_ERROR_FAILURE;
  result.responseStatus = 0;
  return result;
}

AdblockPlus::ServerResponse
AdblockPlus::DefaultWebRequestSync::HEAD(const std::string& url,
                                         const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = IWebRequest::NS_ERROR_FAILURE;
  result.responseStatus = 0;
  return result;
}

DefaultWebRequest::DefaultWebRequest(IExecutor& executor, WebRequestSyncPtr syncImpl)
    : executor(executor), syncImpl(std::move(syncImpl))
{
}

DefaultWebRequest::~DefaultWebRequest()
{
}

void DefaultWebRequest::GET(const std::string& url,
                            const HeaderList& requestHeaders,
                            const RequestCallback& requestCallback)
{
  executor.Dispatch([this, url, requestHeaders, requestCallback] {
    requestCallback(this->syncImpl->GET(url, requestHeaders));
  });
}

void DefaultWebRequest::HEAD(const std::string& url,
                             const HeaderList& requestHeaders,
                             const RequestCallback& requestCallback)
{
  executor.Dispatch([this, url, requestHeaders, requestCallback] {
    requestCallback(this->syncImpl->HEAD(url, requestHeaders));
  });
}
