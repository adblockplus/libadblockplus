#pragma once

#ifdef HAVE_CURL

#include "../src/DefaultWebRequest.h"

class WebRequestCurl : public AdblockPlus::IWebRequestSync
{
public:
  AdblockPlus::ServerResponse GET(const std::string& url,
                                  const AdblockPlus::HeaderList& requestHeaders) const override;
  AdblockPlus::ServerResponse HEAD(const std::string& url,
                                   const AdblockPlus::HeaderList& requestHeaders) const override;

private:
  void execute(void* curl,
               const std::string& url,
               const AdblockPlus::HeaderList& requestHeaders,
               AdblockPlus::ServerResponse& result) const;
};

#endif // HAVE_CURL
