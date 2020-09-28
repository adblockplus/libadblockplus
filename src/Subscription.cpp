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

#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/Subscription.h>

#include "Utils.h"

using namespace AdblockPlus;

Subscription::Subscription(JsValue&& object, JsEngine* engine)
    : jsObject(std::move(object)), jsEngine(engine)
{
  if (!jsObject.IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

bool Subscription::IsListed() const
{
  JsValue func = jsEngine->Evaluate("API.isListedSubscription");
  return func.Call(jsObject).AsBool();
}

bool Subscription::IsDisabled() const
{
  return jsObject.GetProperty("disabled").AsBool();
}

void Subscription::SetDisabled(bool value)
{
  return jsObject.SetProperty("disabled", value);
}

void Subscription::AddToList()
{
  JsValue func = jsEngine->Evaluate("API.addSubscriptionToList");
  func.Call(jsObject);
}

void Subscription::RemoveFromList()
{
  JsValue func = jsEngine->Evaluate("API.removeSubscriptionFromList");
  func.Call(jsObject);
}

void Subscription::UpdateFilters()
{
  JsValue func = jsEngine->Evaluate("API.updateSubscription");
  func.Call(jsObject);
}

bool Subscription::IsUpdating() const
{
  JsValue func = jsEngine->Evaluate("API.isSubscriptionUpdating");
  return func.Call(jsObject).AsBool();
}

bool Subscription::IsAA() const
{
  return jsEngine->Evaluate("API.isAASubscription").Call(jsObject).AsBool();
}

std::string Subscription::GetTitle() const
{
  return GetStringProperty("title");
}

std::string Subscription::GetUrl() const
{
  return GetStringProperty("url");
}

std::string Subscription::GetHomepage() const
{
  return GetStringProperty("homepage");
}

std::string Subscription::GetAuthor() const
{
  return GetStringProperty("author");
}

std::vector<std::string> Subscription::GetLanguages() const
{
  return Utils::SplitString(GetStringProperty("prefixes"), ',');
}

int Subscription::GetFilterCount() const
{
  return jsObject.GetProperty("filterCount").AsInt();
}

std::string Subscription::GetSynchronizationStatus() const
{
  return GetStringProperty("downloadStatus");
}

bool Subscription::operator==(const Subscription& value) const
{
  return GetUrl() == value.GetUrl();
}

std::string Subscription::GetStringProperty(const std::string& name) const
{
  JsValue value = jsObject.GetProperty(name);
  return (value.IsUndefined() || value.IsNull()) ? "" : value.AsString();
}
