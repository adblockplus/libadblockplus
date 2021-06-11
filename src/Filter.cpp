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
#include <assert.h>

#include <AdblockPlus/Filter.h>

using namespace AdblockPlus;

Filter::Filter() : Filter(nullptr)
{
}

Filter::~Filter() = default;

Filter::Filter(std::unique_ptr<IFilterImplementation> impl) : implementation(std::move(impl))
{
}

Filter::Type Filter::GetType() const
{
  assert(IsValid());
  return implementation->GetType();
}

std::string Filter::GetRaw() const
{
  assert(IsValid());
  return implementation->GetRaw();
}

bool Filter::operator==(const Filter& filter) const
{
  if (implementation == filter.implementation)
    return true;
  if (!implementation || !filter.implementation)
    return false;

  return *(implementation) == *filter.implementation;
}

Filter& Filter::operator=(const Filter& filter)
{
  if (filter.implementation)
    implementation = filter.implementation->Clone();
  return *this;
}

Filter& Filter::operator=(Filter&& filter)
{
  implementation = std::move(filter.implementation);
  return *this;
}

Filter::Filter(const Filter& other)
{
  *this = other;
}

Filter::Filter(Filter&& other)
{
  *this = std::move(other);
}

const IFilterImplementation* Filter::Implementation() const
{
  return implementation.get();
}

bool Filter::IsValid() const
{
  return implementation != nullptr;
}
