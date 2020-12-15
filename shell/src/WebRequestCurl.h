#pragma once

#ifdef HAVE_CURL

#include "DefaultWebRequest.h"

class WebRequestCurl : public AdblockPlus::IWebRequestSync
{
public:
  AdblockPlus::ServerResponse GET(const std::string& url,
                                  const AdblockPlus::HeaderList& requestHeaders) const override;
};

#endif // HAVE_CURL
