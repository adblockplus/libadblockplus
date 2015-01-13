/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

#include <sstream>
#include <cctype>
#include <algorithm>
#include <curl/curl.h>
#include <AdblockPlus/DefaultWebRequest.h>

namespace
{
  struct HeaderData
  {
    int status;
    bool expectingStatus;
    std::vector<std::string> headers;

    HeaderData()
    {
      status = 0;
      expectingStatus = true;
    }
  };

  unsigned int ConvertErrorCode(CURLcode code)
  {
    switch (code)
    {
      case CURLE_OK:
        return AdblockPlus::WebRequest::NS_OK;
      case CURLE_FAILED_INIT:
        return AdblockPlus::WebRequest::NS_ERROR_NOT_INITIALIZED;
      case CURLE_UNSUPPORTED_PROTOCOL:
        return AdblockPlus::WebRequest::NS_ERROR_UNKNOWN_PROTOCOL;
      case CURLE_URL_MALFORMAT:
        return AdblockPlus::WebRequest::NS_ERROR_MALFORMED_URI;
      case CURLE_COULDNT_RESOLVE_PROXY:
        return AdblockPlus::WebRequest::NS_ERROR_UNKNOWN_PROXY_HOST;
      case CURLE_COULDNT_RESOLVE_HOST:
        return AdblockPlus::WebRequest::NS_ERROR_UNKNOWN_HOST;
      case CURLE_COULDNT_CONNECT:
        return AdblockPlus::WebRequest::NS_ERROR_CONNECTION_REFUSED;
      case CURLE_OUT_OF_MEMORY:
        return AdblockPlus::WebRequest::NS_ERROR_OUT_OF_MEMORY;
      case CURLE_OPERATION_TIMEDOUT:
        return AdblockPlus::WebRequest::NS_ERROR_NET_TIMEOUT;
      case CURLE_TOO_MANY_REDIRECTS:
        return AdblockPlus::WebRequest::NS_ERROR_REDIRECT_LOOP;
      case CURLE_GOT_NOTHING:
        return AdblockPlus::WebRequest::NS_ERROR_NO_CONTENT;
      case CURLE_SEND_ERROR:
        return AdblockPlus::WebRequest::NS_ERROR_NET_RESET;
      case CURLE_RECV_ERROR:
        return AdblockPlus::WebRequest::NS_ERROR_NET_RESET;
      default:
        return AdblockPlus::WebRequest::NS_CUSTOM_ERROR_BASE + code;
    }
  }

  size_t ReceiveData(char* ptr, size_t size, size_t nmemb, void* userdata)
  {
    std::stringstream* stream = static_cast<std::stringstream*>(userdata);
    stream->write(ptr, size * nmemb);
    return nmemb;
  }

  size_t ReceiveHeader(char* ptr, size_t size, size_t nmemb, void* userdata)
  {
    HeaderData* data = static_cast<HeaderData*>(userdata);
    std::string header(ptr, size * nmemb);
    if (data->expectingStatus)
    {
      // Parse the status code out of something like "HTTP/1.1 200 OK"
      const std::string prefix("HTTP/1.");
      size_t prefixLen = prefix.length();
      if (header.length() >= prefixLen + 2 && !header.compare(0, prefixLen, prefix) &&
          isdigit(header[prefixLen]) && isspace(header[prefixLen + 1]))
      {
        size_t statusStart = prefixLen + 2;
        while (statusStart < header.length() && isspace(header[statusStart]))
          statusStart++;

        size_t statusEnd = statusStart;
        while (statusEnd < header.length() && isdigit(header[statusEnd]))
          statusEnd++;

        if (statusEnd > statusStart && statusEnd < header.length() &&
            isspace(header[statusEnd]))
        {
          std::istringstream(header.substr(statusStart, statusEnd - statusStart)) >> data->status;
          data->headers.clear();
          data->expectingStatus = false;
        }
      }
    }
    else
    {
      size_t headerEnd = header.length();
      while (headerEnd > 0 && isspace(header[headerEnd - 1]))
        headerEnd--;

      if (headerEnd)
        data->headers.push_back(header.substr(0, headerEnd));
      else
        data->expectingStatus = true;
    }
    return nmemb;
  }
}

AdblockPlus::ServerResponse AdblockPlus::DefaultWebRequest::GET(
    const std::string& url, const HeaderList& requestHeaders) const
{
  AdblockPlus::ServerResponse result;
  result.status = NS_ERROR_NOT_INITIALIZED;
  result.responseStatus = 0;

  CURL *curl = curl_easy_init();
  if (curl)
  {
    std::stringstream responseText;
    HeaderData headerData;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseText);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ReceiveHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);

    struct curl_slist* headerList = 0;
    for (HeaderList::const_iterator it = requestHeaders.begin();
      it != requestHeaders.end(); ++it)
    {
      headerList = curl_slist_append(headerList, (it->first + ": " + it->second).c_str());
    }
    if (headerList)
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

    result.status = ConvertErrorCode(curl_easy_perform(curl));
    result.responseStatus = headerData.status;
    result.responseText = responseText.str();
    for (std::vector<std::string>::iterator it = headerData.headers.begin();
        it != headerData.headers.end(); ++it)
    {
      // Parse header name and value out of something like "Foo: bar"
      std::string header = *it;
      size_t colonPos = header.find(':');
      if (colonPos != std::string::npos)
      {
        size_t nameStart = 0;
        size_t nameEnd = colonPos;
        while (nameEnd > nameStart && isspace(header[nameEnd - 1]))
          nameEnd--;

        size_t valueStart = colonPos + 1;
        while (valueStart < header.length() && isspace(header[valueStart]))
          valueStart++;
        size_t valueEnd = header.length();

        if (nameEnd > nameStart && valueEnd > valueStart)
        {
          std::string name = header.substr(nameStart, nameEnd - nameStart);
          std::transform(name.begin(), name.end(), name.begin(), ::tolower);
          std::string value = header.substr(valueStart, valueEnd - valueStart);
          result.responseHeaders.push_back(std::pair<std::string, std::string>(
              name, value));
        }
      }
    }

    if (headerList)
      curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);
  }
  return result;
}
