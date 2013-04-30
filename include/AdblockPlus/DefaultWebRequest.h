#ifndef ADBLOCK_PLUS_DEFAULT_WEB_REQUEST_H
#define ADBLOCK_PLUS_DEFAULT_WEB_REQUEST_H

#include "WebRequest.h"

namespace AdblockPlus
{
  class DefaultWebRequest : public WebRequest
  {
    ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const;
  };
}

#endif
