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

JsError::JsError(v8::Isolate* isolate,
                 const v8::Local<v8::Value>& exception,
                 const v8::Local<v8::Message>& message)
    : std::runtime_error(ExceptionToString(isolate, exception, message))
{
}

std::string JsError::ExceptionToString(v8::Isolate* isolate,
                                       const v8::Local<v8::Value>& exception,
                                       const v8::Local<v8::Message>& message)
{
  std::stringstream error;
  error << *v8::String::Utf8Value(isolate, exception);
  if (!message.IsEmpty())
  {
    error << " at ";
    error << *v8::String::Utf8Value(isolate, message->GetScriptResourceName());
    error << ":";
    auto maybeLineNumber = message->GetLineNumber(isolate->GetCurrentContext());
    int lineNumber = 0;
    if (maybeLineNumber.To(&lineNumber))
      error << lineNumber;
    else
      error << "unknown line";
  }
  return error.str();
}

JsError::JsError(const char* message, const char* filename, int line)
    : std::runtime_error(ErrorToString(message, filename, line))
{
}

std::string JsError::ErrorToString(const char* message, const char* filename, int line)
{
  std::stringstream error;
  error << message;
  error << filename;
  error << ":";
  error << line;
  return error.str();
}
