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
#include "DefaultFilterImplementation.h"

#include <exception>
#include <string>

#include "JsEngine.h"

using namespace AdblockPlus;

DefaultFilterImplementation::DefaultFilterImplementation(JsValue&& value, JsEngine* engine)
    : jsObject(std::move(value)), jsEngine(engine)
{
  if (!jsObject.IsObject())
    throw std::runtime_error("JavaScript value is not an object");
}

IFilterImplementation::Type DefaultFilterImplementation::GetType() const
{
  std::string className = jsObject.GetClass();
  if (className == "BlockingFilter")
    return TYPE_BLOCKING;
  else if (className == "AllowingFilter")
    return TYPE_EXCEPTION;
  else if (className == "ElemHideFilter")
    return TYPE_ELEMHIDE;
  else if (className == "ElemHideException")
    return TYPE_ELEMHIDE_EXCEPTION;
  else if (className == "ElemHideEmulationFilter")
    return TYPE_ELEMHIDE_EMULATION;
  else if (className == "CommentFilter")
    return TYPE_COMMENT;
  else
    return TYPE_INVALID;
}

std::string DefaultFilterImplementation::GetRaw() const
{
  return GetStringProperty("text");
}

bool DefaultFilterImplementation::operator==(const IFilterImplementation& filter) const
{
  return GetRaw() == filter.GetRaw();
}

std::string DefaultFilterImplementation::GetStringProperty(const std::string& name) const
{
  JsValue value = jsObject.GetProperty(name);
  return (value.IsUndefined() || value.IsNull()) ? "" : value.AsString();
}

std::unique_ptr<IFilterImplementation> DefaultFilterImplementation::Clone() const
{
  auto copyObject = jsObject;
  return std::make_unique<DefaultFilterImplementation>(std::move(copyObject), jsEngine);
}

bool DefaultFilterImplementation::IsListed() const
{
  JsValue func = jsEngine->Evaluate("API.isListedFilter");
  return func.Call(jsObject).AsBool();
}

void DefaultFilterImplementation::AddToList()
{
  JsValue func = jsEngine->Evaluate("API.addFilterToList");
  func.Call(jsObject);
}

void DefaultFilterImplementation::RemoveFromList()
{
  JsValue func = jsEngine->Evaluate("API.removeFilterFromList");
  func.Call(jsObject);
}
