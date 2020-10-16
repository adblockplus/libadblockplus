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

#include "DefaultSubscriptionImplementation.h"

#include <exception>
#include <string>

#include <AdblockPlus/JsEngine.h>

#include "Utils.h"

using namespace AdblockPlus;

DefaultSubscriptionImplementation::DefaultSubscriptionImplementation(JsValue&& object,
                                                                     JsEngine* engine)
    : jsObject(std::move(object)), jsEngine(engine)
{
  if (!jsObject.IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

bool DefaultSubscriptionImplementation::IsDisabled() const
{
  return jsObject.GetProperty("disabled").AsBool();
}

void DefaultSubscriptionImplementation::SetDisabled(bool value)
{
  return jsObject.SetProperty("disabled", value);
}

void DefaultSubscriptionImplementation::UpdateFilters()
{
  JsValue func = jsEngine->Evaluate("API.updateSubscription");
  func.Call(jsObject);
}

bool DefaultSubscriptionImplementation::IsUpdating() const
{
  JsValue func = jsEngine->Evaluate("API.isSubscriptionUpdating");
  return func.Call(jsObject).AsBool();
}

bool DefaultSubscriptionImplementation::IsAA() const
{
  return jsEngine->Evaluate("API.isAASubscription").Call(jsObject).AsBool();
}

std::string DefaultSubscriptionImplementation::GetTitle() const
{
  return GetStringProperty("title");
}

std::string DefaultSubscriptionImplementation::GetUrl() const
{
  return GetStringProperty("url");
}

std::string DefaultSubscriptionImplementation::GetHomepage() const
{
  return GetStringProperty("homepage");
}

std::string DefaultSubscriptionImplementation::GetAuthor() const
{
  return GetStringProperty("author");
}

std::vector<std::string> DefaultSubscriptionImplementation::GetLanguages() const
{
  return Utils::SplitString(GetStringProperty("prefixes"), ',');
}

int DefaultSubscriptionImplementation::GetFilterCount() const
{
  return jsObject.GetProperty("filterCount").AsInt();
}

std::string DefaultSubscriptionImplementation::GetSynchronizationStatus() const
{
  return GetStringProperty("downloadStatus");
}

int DefaultSubscriptionImplementation::GetLastDownloadAttemptTime() const
{
  return GetIntProperty("lastDownload");
}

int DefaultSubscriptionImplementation::GetLastDownloadSuccessTime() const
{
  return GetIntProperty("lastSuccess");
}

bool DefaultSubscriptionImplementation::operator==(const ISubscriptionImplementation& value) const
{
  return GetUrl() == value.GetUrl();
}

std::string DefaultSubscriptionImplementation::GetStringProperty(const std::string& name) const
{
  JsValue value = jsObject.GetProperty(name);
  return (value.IsUndefined() || value.IsNull()) ? "" : value.AsString();
}

int DefaultSubscriptionImplementation::GetIntProperty(const std::string& name) const
{
  JsValue value = jsObject.GetProperty(name);
  return (value.IsUndefined() || value.IsNull()) ? 0 : value.AsInt();
}

std::unique_ptr<ISubscriptionImplementation> DefaultSubscriptionImplementation::Clone() const
{
  JsValue copyObject = jsObject;
  return std::make_unique<DefaultSubscriptionImplementation>(std::move(copyObject), jsEngine);
}

bool DefaultSubscriptionImplementation::IsListed() const
{
  JsValue func = jsEngine->Evaluate("API.isListedSubscription");
  return func.Call(jsObject).AsBool();
}

void DefaultSubscriptionImplementation::AddToList()
{
  JsValue func = jsEngine->Evaluate("API.addSubscriptionToList");
  func.Call(jsObject);
}

void DefaultSubscriptionImplementation::RemoveFromList()
{
  JsValue func = jsEngine->Evaluate("API.removeSubscriptionFromList");
  func.Call(jsObject);
}
