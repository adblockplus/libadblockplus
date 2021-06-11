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

#include <AdblockPlus/Subscription.h>

using namespace AdblockPlus;

Subscription::Subscription(std::unique_ptr<ISubscriptionImplementation> impl)
    : implementation(std::move(impl))
{
  assert(implementation != nullptr);
}

Subscription::~Subscription() = default;

bool Subscription::IsDisabled() const
{
  return implementation->IsDisabled();
}

void Subscription::SetDisabled(bool value)
{
  implementation->SetDisabled(value);
}

void Subscription::UpdateFilters()
{
  implementation->UpdateFilters();
}

bool Subscription::IsUpdating() const
{
  return implementation->IsUpdating();
}

bool Subscription::IsAA() const
{
  return implementation->IsAA();
}

std::string Subscription::GetTitle() const
{
  return implementation->GetTitle();
}

std::string Subscription::GetUrl() const
{
  return implementation->GetUrl();
}

std::string Subscription::GetHomepage() const
{
  return implementation->GetHomepage();
}

std::string Subscription::GetAuthor() const
{
  return implementation->GetAuthor();
}

std::vector<std::string> Subscription::GetLanguages() const
{
  return implementation->GetLanguages();
}

int Subscription::GetFilterCount() const
{
  return implementation->GetFilterCount();
}

std::string Subscription::GetSynchronizationStatus() const
{
  return implementation->GetSynchronizationStatus();
}

int Subscription::GetLastDownloadAttemptTime() const
{
  return implementation->GetLastDownloadAttemptTime();
}

int Subscription::GetLastDownloadSuccessTime() const
{
  return implementation->GetLastDownloadSuccessTime();
}

bool Subscription::operator==(const Subscription& other) const
{
  return *implementation == *(other.implementation);
}

const ISubscriptionImplementation* Subscription::Implementation() const
{
  return implementation.get();
}

Subscription& Subscription::operator=(const Subscription& other)
{
  implementation = other.implementation->Clone();
  return *this;
}

Subscription& Subscription::operator=(Subscription&& other)
{
  implementation = std::move(other.implementation);
  return *this;
}

Subscription::Subscription(const Subscription& other)
{
  operator=(other);
}

Subscription::Subscription(Subscription&& other)
{
  operator=(std::move(other));
}
