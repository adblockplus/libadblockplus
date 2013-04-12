#include <AdblockPlus/WebRequest.h>

AdblockPlus::ServerResponse AdblockPlus::DefaultWebRequest::GET(
    const std::string& url, const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = NS_ERROR_FAILURE;
  result.responseStatus = 0;
  return result;
}
