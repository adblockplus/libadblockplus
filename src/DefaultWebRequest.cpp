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

DefaultWebRequest::DefaultWebRequest(const Scheduler& scheduler, WebRequestSyncPtr syncImpl)
    : scheduler(scheduler), syncImpl(std::move(syncImpl))
{
}

DefaultWebRequest::~DefaultWebRequest()
{
}

void DefaultWebRequest::GET(const std::string& url,
                            const HeaderList& requestHeaders,
                            const RequestCallback& requestCallback)
{
  scheduler([this, url, requestHeaders, requestCallback] {
    requestCallback(this->syncImpl->GET(url, requestHeaders));
  });
}

void DefaultWebRequest::HEAD(const std::string& url,
                             const HeaderList& requestHeaders,
                             const RequestCallback& requestCallback)
{
  scheduler([this, url, requestHeaders, requestCallback] {
    requestCallback(this->syncImpl->HEAD(url, requestHeaders));
  });
}