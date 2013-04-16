#include "WebRequestWinInet.h"
#include <Windows.h>
#include <winhttp.h>
#include <Shlwapi.h>

AdblockPlus::ServerResponse AdblockPlus::WebRequestWinInet::GET(
  const std::string& url, const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = NS_ERROR_FAILURE;
  result.responseStatus = 0;

  HRESULT hr;
  BOOL res;
  bool isSecure = url.find("https://") == 0;

  // UTF-16 representation can't be more then double in size of the UTF-8 one
  DWORD urlWLength = url.length() * 2;
  WCHAR* urlW = new WCHAR[urlWLength];
  urlWLength = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), url.length(), urlW, urlWLength);
  urlW[urlWLength] = L'\0';

  DWORD canonizedUrlLength = 2049;  //de facto max limit for request URL

  //Get the size of canonized URL
  WCHAR* canonizedUrl = new WCHAR[canonizedUrlLength];
  hr = UrlCanonicalize(urlW, canonizedUrl, &canonizedUrlLength, 0);
  if (FAILED(hr))
  {
    // Buffer was too small? Retry with new buffer size
    delete []canonizedUrl;
    canonizedUrl = new WCHAR[canonizedUrlLength];
    hr = UrlCanonicalize(urlW, canonizedUrl, &canonizedUrlLength, 0);
    if (FAILED(res))
    {
      delete []canonizedUrl;
      delete []urlW;
      return result;
    }
  }

  std::string headersString;
  for (int i = 0; i < requestHeaders.size(); i++)
  {
    headersString += requestHeaders[i].first + ": ";
    headersString += requestHeaders[i].second + "\r\n";
  }
  // UTF-16 representation can't be more then double in size of the UTF-8 one
  DWORD headersLength = headersString.length() * 2;
  WCHAR* headers = new WCHAR[headersLength];
  headersLength = MultiByteToWideChar(CP_UTF8, 0, headersString.c_str(), headersString.length(), 0, headersLength);

  LPSTR outBuffer;
  DWORD downloadSize, downloaded;
  HINTERNET  hSession = 0, hConnect = 0, hRequest = 0;

  // Use WinHttpOpen to obtain a session handle.
  // TODO: Make sure the correct proxy gets selected
  hSession = WinHttpOpen(L"Adblock Plus", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

  // Specify an HTTP server.
  if( hSession )
  {
    URL_COMPONENTS urlComponents;
    
    // Initialize the URL_COMPONENTS structure.
    ZeroMemory(&urlComponents, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);

    // Set required component lengths to non-zero so that they are cracked.
    urlComponents.dwSchemeLength    = (DWORD)-1;
    urlComponents.dwHostNameLength  = (DWORD)-1;
    urlComponents.dwUrlPathLength   = (DWORD)-1;
    urlComponents.dwExtraInfoLength = (DWORD)-1;
    res = WinHttpCrackUrl(canonizedUrl, canonizedUrlLength, 0, &urlComponents);
    if (res)
    {
      std::wstring hostName(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
      if (isSecure)
      {
        hConnect = WinHttpConnect(hSession, hostName.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
      }
      else
      {
        hConnect = WinHttpConnect(hSession, hostName.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
      }
      DWORD err = GetLastError();

      // Create an HTTP request handle.
      if( hConnect )
      {
        DWORD flags = 0;
        if (isSecure)
        {
          flags = WINHTTP_FLAG_SECURE;
        }
        hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComponents.lpszUrlPath, 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);

        // Send a request.
        if(hRequest)
        {
          //TODO: Make sure for HTTP 1.1 "Accept-Encoding: gzip, deflate" doesn't HAVE to be set here
          if (headersLength > 0)
          {
            res = ::WinHttpSendRequest(hRequest, headers, headersLength, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
          }
          else
          {
            res = ::WinHttpSendRequest(hRequest, 0, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
          }

          // End the request.
          if(res)
          {
            res = WinHttpReceiveResponse(hRequest, 0);
          }

          DWORD bufLen = 0;
          WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &bufLen, WINHTTP_NO_HEADER_INDEX);
          WCHAR* responseHeadersRaw = new WCHAR[bufLen];
          res = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, responseHeadersRaw, &bufLen, WINHTTP_NO_HEADER_INDEX);
          if (res)
          {
            std::wstring responseHeaders(responseHeadersRaw, bufLen);
            //Iterate through each header. Separator is '\0'
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
              DWORD headerNameMbLength = headerNameW.length() * 2;
              DWORD headerValueMbLength = headerValueW.length() * 2;
              char* headerNameMb = new char[headerNameMbLength];
              char* headerValueMb = new char[headerValueMbLength];
              headerNameMbLength = WideCharToMultiByte(CP_UTF8, 0, headerNameW.c_str(), headerNameW.length(), headerNameMb, headerNameMbLength, 0, 0); 
              headerValueMbLength = WideCharToMultiByte(CP_UTF8, 0, headerValueW.c_str(), headerValueW.length(), headerValueMb, headerValueMbLength, 0, 0);
              result.responseHeaders.push_back(
                std::pair<std::string, std::string>(std::string(headerNameMb, headerNameMbLength), std::string(headerValueMb, headerValueMbLength)));
              delete [] headerNameMb;
              delete [] headerValueMb;
            nextHeaderNameStart ++;
              prevHeaderStart = nextHeaderNameStart;
            }
          }
          delete [] responseHeadersRaw;
          WCHAR statusStr[64];
          DWORD statusLen = sizeof(statusStr);
          res = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, &statusStr, &statusLen, WINHTTP_NO_HEADER_INDEX);
          result.responseStatus = wcstol(statusStr, 0, 10);
          result.status = NS_OK;

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
          // Close any open handles.
          if (hRequest) WinHttpCloseHandle(hRequest);
        }
        if (hConnect) WinHttpCloseHandle(hConnect);
      }
    }
    if (hSession) WinHttpCloseHandle(hSession);
  }


  //Clean up
  delete [] headers;
  delete [] canonizedUrl;
  delete [] urlW;
  return result;
}
