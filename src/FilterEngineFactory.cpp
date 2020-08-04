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

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>

#include <AdblockPlus/FilterEngineFactory.h>
#include "DefaultFilterEngine.h"
#include "JsContext.h"
#include "Thread.h"

using namespace AdblockPlus;

namespace
{
    const std::string filterEngineJsFiles[] =
    {
      "compat.js",
      "info.js",
      "io.js",
      "prefs.js",
      "utils.js",
      "elemHideHitRegistration.js",
      "time.js",
      "events.js",
      "coreUtils.js",
      "caching.js",
      "publicSuffixList.json",
      "subscriptions.json",
      "resources.json",
      "url.js",
      "filterNotifier.js",
      "recommendations.js",
      "common.js",
      "elemHideExceptions.js",
      "contentTypes.js",
      "filterClasses.js",
      "snippets.js",
      "analytics.js",
      "downloader.js",
      "subscriptionClasses.js",
      "iniParser.js",
      "filterStorage.js",
      "filtersByDomain.js",
      "elemHide.js",
      "elemHideEmulation.js",
      "matcher.js",
      "filterListener.js",
      "filterEngine.js",
      "versions.js",
      "notifications.js",
      "notificationShowRegistration.js",
      "synchronizer.js",
      "filterUpdateRegistration.js",
      "jsbn.js",
      "rusha.js",
      "rsa.js",
      "init.js",
      "api.js",
      "punycode.js",
      "uri.js"
    };
}

void FilterEngineFactory::CreateAsync(const JsEnginePtr& jsEngine,
  const EvaluateCallback& evaluateCallback,
  const OnCreatedCallback& onCreated,
  const CreationParameters& params)
{
  std::shared_ptr<DefaultFilterEngine> filterEngine(new DefaultFilterEngine(jsEngine));
  {
    // TODO: replace weakFilterEngine by this when it's possible to control the
    // execution time of the asynchronous part below.
    std::weak_ptr<DefaultFilterEngine> weakFilterEngine = filterEngine;
    auto isSubscriptionDownloadAllowedCallback = params.isSubscriptionDownloadAllowedCallback;
    jsEngine->SetEventCallback("_isSubscriptionDownloadAllowed", [weakFilterEngine, isSubscriptionDownloadAllowedCallback](JsValueList&& params){
      auto filterEngine = weakFilterEngine.lock();
      if (!filterEngine)
        return;
      auto& jsEngine = filterEngine->GetJsEngine();

      // param[0] - nullable string Prefs.allowed_connection_type
      // param[1] - function(Boolean)
      bool areArgumentsValid = params.size() == 2 && (params[0].IsNull() || params[0].IsString()) && params[1].IsFunction();
      assert(areArgumentsValid && "Invalid argument: there should be two args and the second one should be a function");
      if (!areArgumentsValid)
        return;
      if (!isSubscriptionDownloadAllowedCallback)
      {
        params[1].Call(jsEngine.NewValue(true));
        return;
      }
      auto valuesID = jsEngine.StoreJsValues(params);
      auto callJsCallback = [weakFilterEngine, valuesID](bool isAllowed)
      {
        auto filterEngine = weakFilterEngine.lock();
        if (!filterEngine)
          return;
        auto& jsEngine = filterEngine->GetJsEngine();
        auto jsParams = jsEngine.TakeJsValues(valuesID);
        jsParams[1].Call(jsEngine.NewValue(isAllowed));
      };
      std::string allowedConnectionType = params[0].IsString() ? params[0].AsString() : std::string();
      isSubscriptionDownloadAllowedCallback(params[0].IsString() ? &allowedConnectionType : nullptr, callJsCallback);
    });
  }

  jsEngine->SetEventCallback("_init", [jsEngine, filterEngine, onCreated](JsValueList&& params)
  {
    filterEngine->SetIsFirstRun(params.size() && params[0].AsBool());
    onCreated(filterEngine);
    jsEngine->RemoveEventCallback("_init");
  });

  std::weak_ptr<DefaultFilterEngine> weakFilterEngine = filterEngine;
  filterEngine->SetFilterChangeCallback([weakFilterEngine](const std::string& reason, JsValue&&)
  {
    auto filterEngine = weakFilterEngine.lock();
    if (!filterEngine)
      return;
    if (reason == "save")
      filterEngine->GetJsEngine().NotifyLowMemory();
  });

  // Lock the JS engine while we are loading scripts, no timeouts should fire
  // until we are done.
  const JsContext context(*jsEngine);
  // Set the preconfigured prefs
  auto preconfiguredPrefsObject = jsEngine->NewObject();
  for (const auto& pref : params.preconfiguredPrefs)
  {
    preconfiguredPrefsObject.SetProperty(pref.first, pref.second);
  }
  jsEngine->SetGlobalProperty("_preconfiguredPrefs", preconfiguredPrefsObject);

  // Load adblockplus scripts
  for (const auto& filterEngineJsFile: filterEngineJsFiles)
    evaluateCallback(filterEngineJsFile);

}
