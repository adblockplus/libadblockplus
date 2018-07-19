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

#ifndef ADBLOCK_PLUS_UPDATER_H
#define ADBLOCK_PLUS_UPDATER_H

#include <functional>
#include <string>
#include <AdblockPlus/JsEngine.h>

namespace AdblockPlus
{
  /**
   * Component of libadblockplus responsible for Update checks for the application.
   */
  class Updater
  {
  public:
    explicit Updater(const JsEnginePtr& jsEngine, const JsEngine::EvaluateCallback& callback);

    /**
     * Callback type invoked when an update becomes available.
     * The parameter is the download URL of the update.
     */
    typedef std::function<void(const std::string&)> UpdateAvailableCallback;

    /**
     * Callback type invoked when a manually triggered update check finishes.
     * The parameter is an optional error message.
     */
    typedef std::function<void(const std::string&)> UpdateCheckDoneCallback;

    /**
     * Sets the callback invoked when an application update becomes available.
     * @param callback Callback to invoke.
     */
    void SetUpdateAvailableCallback(const UpdateAvailableCallback& callback);

    /**
     * Removes the callback invoked when an application update becomes
     * available.
     */
    void RemoveUpdateAvailableCallback();

    /**
     * Forces an immediate update check.
     * `Updater` will automatically check for updates in regular intervals,
     * so applications should only call this when the user triggers an update
     * check manually.
     * @param callback Optional callback to invoke when the update check is
     *        finished. The string parameter will be empty when the update check
     *        succeeded, or contain an error message if it failed.
     *        Note that the callback will be invoked whether updates are
     *        available or not - to react to updates being available, use
     *        `Updater::SetUpdateAvailableCallback()`.
     */
    void ForceUpdateCheck(const UpdateCheckDoneCallback& callback = UpdateCheckDoneCallback());

  private:
    JsEnginePtr jsEngine;
    int updateCheckId;
  };
}

#endif
