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

#include <AdblockPlus.h>
#include "Utils.h"
#include <cassert>

using namespace AdblockPlus;

namespace {
    static std::string updaterJsFiles[] = {
      "compat.js",
      "info.js",
      "prefs.js",
      "utils.js",
      "events.js",
      "coreUtils.js",
      "common.js",
      "downloader.js",
      "updater.js",
      "apiUpdater.js"
    };
}

Updater::Updater(const JsEnginePtr& jsEngine, const JsEngine::EvaluateCallback& evaluateCallback)
  : jsEngine(jsEngine), updateCheckId(0)
{
  // Hack to enable downloads from JS (see compat.js)
  jsEngine->SetEventCallback("_isSubscriptionDownloadAllowed", [this](JsValueList&& params)
  {
    // param[0] - nullable string Prefs.allowed_connection_type
    // param[1] - function(Boolean)
    bool areArgumentsValid = params.size() == 2 && (params[0].IsNull() || params[0].IsString()) && params[1].IsFunction();
    assert(areArgumentsValid && "Invalid argument: there should be two args and the second one should be a function");
    if (!areArgumentsValid)
      return;
    params[1].Call(this->jsEngine->NewValue(true));
  });

  // Set empty preconfigured prefs
  auto preconfiguredPrefsObject = jsEngine->NewObject();
  jsEngine->SetGlobalProperty("_preconfiguredPrefs", preconfiguredPrefsObject);

  // Load adblockplus scripts
  for(size_t i = 0; i < ArraySize(updaterJsFiles); ++i)
    evaluateCallback(updaterJsFiles[i]);
}

void Updater::SetUpdateAvailableCallback(const Updater::UpdateAvailableCallback& callback)
{
  jsEngine->SetEventCallback("updateAvailable", [this, callback](JsValueList&& params)
  {
    if (params.size() >= 1 && !params[0].IsNull())
      callback(params[0].AsString());
  });
}

void Updater::RemoveUpdateAvailableCallback()
{
  jsEngine->RemoveEventCallback("updateAvailable");
}

void Updater::ForceUpdateCheck(const Updater::UpdateCheckDoneCallback& callback)
{
  JsValue func = jsEngine->Evaluate("API_UPDATER.forceUpdateCheck");
  JsValueList params;
  if (callback)
  {
    std::string eventName = "_updateCheckDone" + std::to_string(++updateCheckId);
    jsEngine->SetEventCallback(eventName, [this, eventName, callback](JsValueList&& params)
    {
      std::string error(params.size() >= 1 && !params[0].IsNull() ? params[0].AsString() : "");
      callback(error);
      jsEngine->RemoveEventCallback(eventName);
    });
    params.push_back(jsEngine->NewValue(eventName));
  }
  func.Call(params);
}

JsValue Updater::GetPref(const std::string& pref) const
{
  JsValue func = jsEngine->Evaluate("API_UPDATER.getPref");
  return func.Call(jsEngine->NewValue(pref));
}

void Updater::SetPref(const std::string& pref, const JsValue& value)
{
  JsValue func = jsEngine->Evaluate("API_UPDATER.setPref");
  JsValueList params;
  params.push_back(jsEngine->NewValue(pref));
  params.push_back(value);
  func.Call(params);
}
