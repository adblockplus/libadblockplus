#ifndef ADBLOCKPLUS_WEB_REQUEST_H
#define ADBLOCKPLUS_WEB_REQUEST_H

#include <string>
#include <vector>

#include "tr1_memory.h"

namespace AdblockPlus
{
  typedef std::vector<std::pair<std::string, std::string> > HeaderList;

  struct ServerResponse
  {
#ifdef _WIN32
    __int64 status;
#else
    int64_t status;
#endif
    HeaderList responseHeaders;
    int responseStatus;
    std::string responseText;
  };

  class WebRequest
  {
  public:
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
      NS_ERROR_NOT_INITIALIZED = 0xc1f30001,
      NS_CUSTOM_ERROR_BASE = 0x80850000
    };

    virtual inline ~WebRequest() {};
    virtual ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const = 0;
  };

  typedef std::tr1::shared_ptr<WebRequest> WebRequestPtr;
}

#endif
