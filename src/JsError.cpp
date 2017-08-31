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

#include "JsError.h"

using namespace AdblockPlus;

JsError::JsError(const v8::Handle<v8::Value>& exception,
    const v8::Handle<v8::Message>& message)
  : std::runtime_error(ExceptionToString(exception, message))
{
}

std::string JsError::ExceptionToString(const v8::Handle<v8::Value>& exception,
  const v8::Handle<v8::Message>& message)
{
  std::stringstream error;
  error << *v8::String::Utf8Value(exception);
  if (!message.IsEmpty())
  {
    error << " at ";
    error << *v8::String::Utf8Value(message->GetScriptResourceName());
    error << ":";
    error << message->GetLineNumber();
  }
  return error.str();
}