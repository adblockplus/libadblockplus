/*
* This file is part of Adblock Plus <https://adblockplus.org/>,
* Copyright (C) 2006-2017 eyeo GmbH
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

#include "BaseJsTest.h"

JsEngineCreationParameters::JsEngineCreationParameters()
  : logSystem(std::make_shared<ThrowingLogSystem>())
  , timer(new ThrowingTimer())
  , webRequest(new ThrowingWebRequest())
  , fileSystem(std::make_shared<ThrowingFileSystem>())
{
}

AdblockPlus::JsEnginePtr CreateJsEngine(JsEngineCreationParameters&& jsEngineCreationParameters)
{
  static AdblockPlus::ScopedV8IsolatePtr isolate = std::make_shared<AdblockPlus::ScopedV8Isolate>();
  auto jsEngine = AdblockPlus::JsEngine::New(jsEngineCreationParameters.appInfo,
    std::move(jsEngineCreationParameters.timer),
    std::move(jsEngineCreationParameters.webRequest),
    isolate);
  jsEngine->SetLogSystem(std::move(jsEngineCreationParameters.logSystem));
  jsEngine->SetFileSystem(std::move(jsEngineCreationParameters.fileSystem));
  return jsEngine;
}
