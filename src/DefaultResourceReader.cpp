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

#include "DefaultResourceReader.h"

using namespace AdblockPlus;

StringPreloadedFilterResponse::StringPreloadedFilterResponse(std::string data)
    : data(std::move(data))
{
}

bool StringPreloadedFilterResponse::exists() const
{
  return !data.empty();
}

const char* StringPreloadedFilterResponse::content() const
{
  return data.c_str();
}

size_t StringPreloadedFilterResponse::size() const
{
  return data.size();
}

void DefaultResourceReader::ReadPreloadedFilterList(const std::string& url,
                                                    const ReadCallback& doneCallback) const
{
  doneCallback(std::make_unique<StringPreloadedFilterResponse>());
}
