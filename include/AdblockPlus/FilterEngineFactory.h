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

#ifndef ADBLOCK_PLUS_FILTER_ENGINE_FACTORY_H
#define ADBLOCK_PLUS_FILTER_ENGINE_FACTORY_H

#include <functional>
#include <string>
#include <unordered_map>

#include <AdblockPlus/IFilterEngine.h>

namespace AdblockPlus
{
  class FilterEngineFactory
  {
  public:
    /**
     * Container of name-value pairs representing a set of preferences.
     * This is default configuration, values can be overridden with actual state from local
     * preferences storage. Following keys are available:
     * - filter_engine_enabled - default: true, @see IFilterEngine::SetEnabled
     * - first_run_subscription_auto_select - default: true. Will automatically subscribe to filter
     * lists matching AppInfo::locale and Acceptable Ads on first run with enabled FilterEngine if
     * true.
     * - allowed_connection_type - default: "", @see IFilterEngine::SetAllowedConnectionType,
     * IsConnectionAllowedAsyncCallback
     */
    enum class BooleanPrefName
    {
      FilterEngineEnabled,
      FirstRunSubscriptionAutoselect
    };

    enum class StringPrefName
    {
      AllowedConnectionType
    };

    static std::string PrefNameToString(StringPrefName prefName);
    static std::string PrefNameToString(BooleanPrefName prefName);
    static bool StringToPrefName(const std::string& prefNameStr, StringPrefName& prefName);
    static bool StringToPrefName(const std::string& prefNameStr, BooleanPrefName& prefName);

    typedef std::unordered_map<StringPrefName, std::string> StringPrefs;
    typedef std::unordered_map<BooleanPrefName, bool> BooleanPrefs;
    /**
     * Asynchronous callback function passing false when current connection
     * type does not correspond to allowedConnectionType, e.g. because it is a
     * metered connection.
     */
    typedef std::function<void(const std::string* allowedConnectionType,
                               const std::function<void(bool)>&)>
        IsConnectionAllowedAsyncCallback;

    /**
     * IFilterEngine creation parameters.
     */
    struct CreationParameters
    {
      /**
       * `AdblockPlus::FilterEngineFactory::Prefs` name - value list of preconfigured
       * prefs.
       */
      struct Prefs
      {
        void clear()
        {
          stringPrefs.clear();
          booleanPrefs.clear();
        }
        StringPrefs stringPrefs;
        BooleanPrefs booleanPrefs;
      } preconfiguredPrefs;

      /**
       * A callback of `AdblockPlus::IFilterEngine::IsConnectionAllowedAsyncCallback` type
       * checking whether the request to download a subscription from Adblock Plus may be performed
       * on the current connection.
       */
      IsConnectionAllowedAsyncCallback isSubscriptionDownloadAllowedCallback;
    };

    /**
     * Callback type invoked when IFilterEngine is created.
     */
    typedef std::function<void(std::unique_ptr<IFilterEngine>)> OnCreatedCallback;

    /**
     * Callback type for evaluating JS expression.
     * The parameter is the JS file name containing the expression.
     */
    typedef std::function<void(const std::string&)> EvaluateCallback;

    /**
     * Asynchronously constructs IFilterEngine.
     * @param jsEngine `JsEngine` instance used to run JavaScript code
     *        internally.
     * @param onCreated A callback which is called when IFilterEngine is ready
     *        for use.
     * @param parameters optional creation parameters.
     */
    static void CreateAsync(JsEngine& jsEngine,
                            const EvaluateCallback& evaluateCallback,
                            const OnCreatedCallback& onCreated,
                            const CreationParameters& parameters = CreationParameters());
  };
}

#endif
