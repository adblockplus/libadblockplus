/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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

#include <algorithm>
#include <exception>
#include <map>

#include <AdblockPlus/FilterEngine.h>

using namespace AdblockPlus;

namespace {

  typedef std::map<FilterEngine::ContentType, std::string> ContentTypeMap;

  // TODO(mpawlowski) this should be a static array, not map, since we're searching
  // in both directions.
  ContentTypeMap CreateContentTypeMap()
  {
    ContentTypeMap contentTypes;
    contentTypes[FilterEngine::CONTENT_TYPE_OTHER] = "OTHER";
    contentTypes[FilterEngine::CONTENT_TYPE_SCRIPT] = "SCRIPT";
    contentTypes[FilterEngine::CONTENT_TYPE_IMAGE] = "IMAGE";
    contentTypes[FilterEngine::CONTENT_TYPE_STYLESHEET] = "STYLESHEET";
    contentTypes[FilterEngine::CONTENT_TYPE_OBJECT] = "OBJECT";
    contentTypes[FilterEngine::CONTENT_TYPE_SUBDOCUMENT] = "SUBDOCUMENT";
    contentTypes[FilterEngine::CONTENT_TYPE_WEBSOCKET] = "WEBSOCKET";
    contentTypes[FilterEngine::CONTENT_TYPE_WEBRTC] = "WEBRTC";
    contentTypes[FilterEngine::CONTENT_TYPE_PING] = "PING";
    contentTypes[FilterEngine::CONTENT_TYPE_XMLHTTPREQUEST] = "XMLHTTPREQUEST";
    contentTypes[FilterEngine::CONTENT_TYPE_FONT] = "FONT";
    contentTypes[FilterEngine::CONTENT_TYPE_MEDIA] = "MEDIA";
    contentTypes[FilterEngine::CONTENT_TYPE_POPUP] = "POPUP";
    contentTypes[FilterEngine::CONTENT_TYPE_DOCUMENT] = "DOCUMENT";
    contentTypes[FilterEngine::CONTENT_TYPE_GENERICBLOCK] = "GENERICBLOCK";
    contentTypes[FilterEngine::CONTENT_TYPE_ELEMHIDE] = "ELEMHIDE";
    contentTypes[FilterEngine::CONTENT_TYPE_GENERICHIDE] = "GENERICHIDE";
    return contentTypes;
  }
}

const ContentTypeMap contentTypes = CreateContentTypeMap();

std::string FilterEngine::ContentTypeToString(ContentType contentType)
{
  ContentTypeMap::const_iterator it = contentTypes.find(contentType);
  if (it != contentTypes.end())
    return it->second;
  throw std::invalid_argument("Argument is not a valid ContentType");
}

FilterEngine::ContentType FilterEngine::StringToContentType(const std::string& contentType)
{
  std::string contentTypeUpper = contentType;
  std::transform(contentType.begin(), contentType.end(), contentTypeUpper.begin(), ::toupper);
  for (const auto& contentType : contentTypes)
  {
    if (contentType.second == contentTypeUpper)
      return contentType.first;
  }
  throw std::invalid_argument("Cannot convert argument to ContentType");
}
