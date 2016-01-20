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

#ifndef ADBLOCK_PLUS_WEB_REQUEST_H
#define ADBLOCK_PLUS_WEB_REQUEST_H

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace AdblockPlus
{
  /**
   * List of HTTP headers.
   */
  typedef std::vector<std::pair<std::string, std::string> > HeaderList;

  /**
   * HTTP response.
   */
  struct ServerResponse
  {
    //@{
    /**
     * [Mozilla status code](https://developer.mozilla.org/en/docs/Table_Of_Errors#Network_Errors)
     * indicating the network-level request status.
     * This should be 0 (NS_OK) if the request succeeded. Note that this should
     * be NS_OK if the server responded with an error code like "404 Not Found".
     */
#ifdef _WIN32
    __int64 status;
#else
    int64_t status;
#endif
    //@}

    /**
     * List of response headers.
     */
    HeaderList responseHeaders;

    /**
     * HTTP status of the response (e.g.\ 404).
     */
    int responseStatus;

    /**
     * Body text of the response.
     */
    std::string responseText;
  };

  /**
   * Web request interface.
   */
  class WebRequest
  {
  public:
    /**
     * Possible [Mozilla status codes](https://developer.mozilla.org/en/docs/Table_Of_Errors#Network_Errors).
     */
    enum
    {
      NS_OK = 0,
      NS_ERROR_FAILURE = 0x80004005,
      NS_ERROR_OUT_OF_MEMORY = 0x8007000e,
      NS_ERROR_MALFORMED_URI = 0x804b000a,
      NS_ERROR_CONNECTION_REFUSED = 0x804b000d,
      NS_ERROR_NET_TIMEOUT = 0x804b000e,
      NS_ERROR_NO_CONTENT = 0x804b0011,
      NS_ERROR_UNKNOWN_PROTOCOL = 0x804b0012,
      NS_ERROR_NET_RESET = 0x804b0014,
      NS_ERROR_UNKNOWN_HOST = 0x804b001e,
      NS_ERROR_REDIRECT_LOOP = 0x804b001f,
      NS_ERROR_UNKNOWN_PROXY_HOST = 0x804b002a,
      NS_ERROR_NET_INTERRUPT = 0x804b0047,
      NS_ERROR_UNKNOWN_PROXY_CONNECTION_REFUSED = 0x804b0048,
      NS_CUSTOM_ERROR_BASE = 0x80850000,
      NS_ERROR_NOT_INITIALIZED = 0xc1f30001
    };

    virtual inline ~WebRequest() {};

    /**
     * Performs a GET request.
     * @param url Request URL.
     * @param requestHeaders Request headers.
     * @return HTTP response.
     */
    virtual ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const = 0;
  };

  /**
   * Shared smart pointer to a `WebRequest` instance.
   */
  typedef std::shared_ptr<WebRequest> WebRequestPtr;
}

#endif
