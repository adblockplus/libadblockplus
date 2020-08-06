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

#include <exception>
#include <string>

#include <AdblockPlus/Subscription.h>
#include <AdblockPlus/JsEngine.h>

using namespace AdblockPlus;

Subscription::Subscription(const Subscription& src)
  : JsValue(src)
{
}

Subscription::Subscription(Subscription&& src)
  : JsValue(std::move(src))
{
}

Subscription::Subscription(JsValue&& value)
    : JsValue(std::move(value))
{
  if (!IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

Subscription& Subscription::operator=(const Subscription& src)
{
  static_cast<JsValue&>(*this) = src;
  return *this;
}

Subscription& Subscription::operator=(Subscription&& src)
{
  static_cast<JsValue&>(*this) = std::move(src);
  return *this;
}

bool Subscription::IsListed() const
{
  JsValue func = jsEngine->Evaluate("API.isListedSubscription");
  return func.Call(*this).AsBool();
}

bool Subscription::IsDisabled() const
{
  return GetProperty("disabled").AsBool();
}

void Subscription::SetDisabled(bool value)
{
  return SetProperty("disabled", value);
}

void Subscription::AddToList()
{
  JsValue func = jsEngine->Evaluate("API.addSubscriptionToList");
  func.Call(*this);
}

void Subscription::RemoveFromList()
{
  JsValue func = jsEngine->Evaluate("API.removeSubscriptionFromList");
  func.Call(*this);
}

void Subscription::UpdateFilters()
{
  JsValue func = jsEngine->Evaluate("API.updateSubscription");
  func.Call(*this);
}

bool Subscription::IsUpdating() const
{
  JsValue func = jsEngine->Evaluate("API.isSubscriptionUpdating");
  return func.Call(*this).AsBool();
}

bool Subscription::IsAA() const
{
  return jsEngine->Evaluate("API.isAASubscription").Call(*this).AsBool();
}

bool Subscription::operator==(const Subscription& subscription) const
{
  return GetProperty("url").AsString() == subscription.GetProperty("url").AsString();
}
