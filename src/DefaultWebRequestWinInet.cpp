#include "AdblockPlus\WebRequest.h"
#include <algorithm>
#include <Windows.h>
#include <winhttp.h>
#include <Shlwapi.h>

#include "Utils.h"


BOOL GetProxySettings(std::wstring& proxyName, std::wstring& proxyBypass)
{
  BOOL bResult = TRUE;

  // Get Proxy config info.
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;

  ::ZeroMemory(&proxyConfig, sizeof(proxyConfig));

  if (WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
  {
    if (proxyConfig.lpszProxy != 0)
    {
      proxyName.assign(proxyConfig.lpszProxy);
    }
    if (proxyConfig.lpszProxyBypass != 0)
    {
      proxyBypass.assign(proxyConfig.lpszProxyBypass);
    }
  }
  else
  {
      bResult = FALSE;
  }

  // The strings need to be freed.
  if (proxyConfig.lpszProxy != NULL)
  {
    ::GlobalFree(proxyConfig.lpszProxy);
  }
  if (proxyConfig.lpszAutoConfigUrl != NULL)
  {
    ::GlobalFree(proxyConfig.lpszAutoConfigUrl);
  }
  if (proxyConfig.lpszProxyBypass!= NULL)
  {
    ::GlobalFree(proxyConfig.lpszProxyBypass);
  }

  return bResult;
}


AdblockPlus::ServerResponse AdblockPlus::DefaultWebRequest::GET(
  const std::string& url, const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = NS_ERROR_FAILURE;
  result.responseStatus = 0;

  HRESULT hr;
  BOOL res;

  std::wstring canonizedUrl = Utils::CanonizeUrl(Utils::ToUTF16String(url, url.length()));

  std::string headersString;
  for (int i = 0; i < requestHeaders.size(); i++)
  {
    headersString += requestHeaders[i].first + ": ";
    headersString += requestHeaders[i].second + "\r\n";
  }
  std::wstring headers = Utils::ToUTF16String(headersString, headersString.length());

  LPSTR outBuffer;
  DWORD downloadSize, downloaded;
  HINTERNET  hSession = 0, hConnect = 0, hRequest = 0;

  // Use WinHttpOpen to obtain a session handle.
  std::wstring proxyName, proxyBypass;
  
  GetProxySettings(proxyName, proxyBypass);
  if (proxyName.empty())
  {
    hSession = WinHttpOpen(L"Adblock Plus", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  }
  else
  {
    hSession = WinHttpOpen(L"Adblock Plus", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, proxyName.c_str(), proxyBypass.c_str(), 0);
  }
  // Specify an HTTP server.
  if (!hSession)
  {
    result.status = NS_CUSTOM_ERROR_BASE;
    return result;
  }
  URL_COMPONENTS urlComponents;

  // Initialize the URL_COMPONENTS structure.
  ZeroMemory(&urlComponents, sizeof(urlComponents));
  urlComponents.dwStructSize = sizeof(urlComponents);

  // Set required component lengths to non-zero so that they are cracked.
  urlComponents.dwSchemeLength    = (DWORD)-1;
  urlComponents.dwHostNameLength  = (DWORD)-1;
  urlComponents.dwUrlPathLength   = (DWORD)-1;
  urlComponents.dwExtraInfoLength = (DWORD)-1;
  res = WinHttpCrackUrl(canonizedUrl.c_str(), canonizedUrl.length(), 0, &urlComponents);
  if (!res)
  {
    result.status = NS_ERROR_MALFORMED_URI;
    return result;
  }
  std::wstring hostName(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
  bool isSecure = urlComponents.nScheme == INTERNET_SCHEME_HTTPS;
  if (isSecure)
  {
    hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComponents.nPort, 0);
  }
  else
  {
    hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComponents.nPort, 0);
  }
  DWORD err = GetLastError();

  // Create an HTTP request handle.
  if (!hConnect)
  {
    if (hSession) WinHttpCloseHandle(hSession);
    result.status = NS_ERROR_UNKNOWN_HOST;
    return result;
  }
  DWORD flags = 0;
  if (isSecure)
  {
    flags = WINHTTP_FLAG_SECURE;
  }
  hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComponents.lpszUrlPath, 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

  // Send a request.
  if (!hRequest)
  {
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    result.status = NS_ERROR_FAILURE;
    return result;
  }
  // TODO: Make sure for HTTP 1.1 "Accept-Encoding: gzip, deflate" doesn't HAVE to be set here
  if (headers.length() > 0)
  {
    res = ::WinHttpSendRequest(hRequest, headers.c_str(), headers.length(), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  }
  else
  {
    res = ::WinHttpSendRequest(hRequest, 0, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  }

  // End the request.
  if(res)
  {
    res = WinHttpReceiveResponse(hRequest, 0);
    if (!res)
    {
      if (hRequest) WinHttpCloseHandle(hRequest);
      if (hConnect) WinHttpCloseHandle(hConnect);
      if (hSession) WinHttpCloseHandle(hSession);
      result.status = NS_ERROR_NO_CONTENT;
      return result;
    }
  }

  // Parse the response headers
  DWORD bufLen = 0;
  WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &bufLen, WINHTTP_NO_HEADER_INDEX);
  WCHAR* responseHeadersRaw = new WCHAR[bufLen];
  res = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, responseHeadersRaw, &bufLen, WINHTTP_NO_HEADER_INDEX);
  if (res)
  {
    std::wstring responseHeaders(responseHeadersRaw, bufLen);
    // Iterate through each header. Separator is '\0'
    int nextHeaderNameStart = 0;
    int headerNameEnd = 0;
    int headerValueStart = 0;
    int prevHeaderStart = 0;
    while ((nextHeaderNameStart = responseHeaders.find(L'\0', nextHeaderNameStart)) > 0)
    {
      headerNameEnd = responseHeaders.find(L": ", prevHeaderStart);
      headerValueStart = headerNameEnd + strlen(": ");
      if ((headerNameEnd > nextHeaderNameStart) || (headerNameEnd < 0))
      {
        nextHeaderNameStart ++;
        prevHeaderStart = nextHeaderNameStart;
        continue;
      }
      std::wstring headerNameW = responseHeaders.substr(prevHeaderStart, headerNameEnd - prevHeaderStart);
      std::wstring headerValueW = responseHeaders.substr(headerValueStart, nextHeaderNameStart - headerValueStart);

      std::string headerName = Utils::ToUTF8String(headerNameW.c_str(), headerNameW.length());
      std::string headerValue = Utils::ToUTF8String(headerValueW.c_str(), headerValueW.length());

      std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
      std::transform(headerValue.begin(), headerValue.end(), headerValue.begin(), ::tolower);

      result.responseHeaders.push_back(
        std::pair<std::string, std::string>(headerName, headerValue));

      nextHeaderNameStart ++;
      prevHeaderStart = nextHeaderNameStart;
    }
  }
  delete [] responseHeadersRaw;

  // Get the response status code
  WCHAR statusStr[64];
  DWORD statusLen = sizeof(statusStr);
  res = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, &statusStr, &statusLen, WINHTTP_NO_HEADER_INDEX);
  result.responseStatus = wcstol(statusStr, 0, 10);
  result.status = NS_OK;

  // Download the actual response
  // Keep checking for data until there is nothing left.
  if(res)
  {
    do 
    {
      // Check for available data.
      downloadSize = 0;
      if( !WinHttpQueryDataAvailable(hRequest, &downloadSize))
      {
        result.responseStatus = NS_ERROR_FAILURE;
        break;
      }
      // Allocate space for the buffer.
      outBuffer = new char[downloadSize+1];
      if (!outBuffer)
      {
        //Out of memory?
        downloadSize=0;
        res = FALSE;
        break;
      }
      else
      {
        // Read the data.
        ZeroMemory(outBuffer, downloadSize+1);

        if( WinHttpReadData(hRequest, (LPVOID)outBuffer, downloadSize, &downloaded))
        {
          result.responseText.append(outBuffer, downloaded);
        }
        // Free the memory allocated to the buffer.
        delete [] outBuffer;
      }
    } while (downloadSize > 0);
  }
  // Clean up
  // Close any open handles.
  if (hRequest) WinHttpCloseHandle(hRequest);
  if (hConnect) WinHttpCloseHandle(hConnect);
  if (hSession) WinHttpCloseHandle(hSession);


  return result;
}
