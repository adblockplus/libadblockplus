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

"use strict";

const {Prefs} = require("prefs");
const {filterNotifier} = require("filterNotifier");

let filtersInitDone = false;

let checkInitialized = () =>
{
  if (Prefs.initialized && filtersInitDone)
  {
    checkInitialized = () => {};
    _triggerEvent("_init", require("filterStorage").FilterStorage.firstRun);
  }
};

Prefs._initListener = function()
{
  checkInitialized();
};

if (Prefs.initialized)
  checkInitialized();

filterNotifier.on("load", () =>
{
  let {FilterStorage} = require("filterStorage");
  if (FilterStorage.firstRun)
  {
    // No data, must be a new user or someone with corrupted data - initialize
    // with default settings

    const {Subscription, DownloadableSubscription} =
      require("subscriptionClasses");
    const {Synchronizer} = require("synchronizer");
    const {Utils} = require("utils");

    if (Prefs.first_run_subscription_auto_select)
    {
      let subscriptions = require("subscriptions.xml");
      let node = Utils.chooseFilterSubscription(subscriptions);
      if (node)
      {
        let subscription = Subscription.fromURL(node.url);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        FilterStorage.addSubscription(subscription);
        if (subscription instanceof DownloadableSubscription &&
            !subscription.lastDownload)
          Synchronizer.execute(subscription);
      }

      let aaSubscription =
          Subscription.fromURL(Prefs.subscriptions_exceptionsurl);
      aaSubscription.disabled = false;
      FilterStorage.addSubscription(aaSubscription);
      if (aaSubscription instanceof DownloadableSubscription &&
          !aaSubscription.lastDownload)
        Synchronizer.execute(aaSubscription);
    }
  }

  filtersInitDone = true;
  checkInitialized();
});
