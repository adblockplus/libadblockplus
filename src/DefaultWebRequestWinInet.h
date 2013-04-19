#ifndef ADBLOCKPLUS_WEB_REQUEST_WININET_H
#define ADBLOCKPLUS_WEB_REQUEST_WININET_H

#include "AdblockPlus\WebRequest.h"

namespace AdblockPlus
{
  class DefaultWebRequest : public WebRequest
  {
    ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const;
  };
}
#endif