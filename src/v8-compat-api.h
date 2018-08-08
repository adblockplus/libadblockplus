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

#pragma once

#include <v8.h>

namespace AdblockPlus
{
  namespace V8CompatApi
  {
    v8::Local<v8::StackFrame> StackTrace_GetFrame(v8::Isolate* isolate, const v8::Local<v8::StackTrace>& stackTrace, uint32_t index)
    {
#if V8_MAJOR_VERSION == 6 && V8_MINOR_VERSION <= 7
      return stackTrace->GetFrame(index);
#else
      return stackTrace->GetFrame(isolate, index);
#endif
    }
  }
}