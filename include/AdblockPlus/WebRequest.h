#ifndef ADBLOCKPLUS_WEB_REQUEST_H
#define ADBLOCKPLUS_WEB_REQUEST_H

#include <string>
#include <vector>

namespace AdblockPlus
{
  typedef std::vector<std::pair<std::string, std::string> > HeaderList;

  struct ServerResponse
  {
    HeaderList responseHeaders;
    int responseStatus;
    std::string responseText;
  };

  class WebRequest
  {
  public:
    virtual ~WebRequest();
    virtual ServerResponse GET(const std::string& url, const HeaderList& requestHeaders) const = 0;
  };
}

#endif
