/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

let {Prefs} = require("prefs");
let {FilterNotifier} = require("filterNotifier");

let prefsInitDone = false;
let filtersInitDone = false;

function checkInitialized()
{
  if (prefsInitDone && filtersInitDone)
  {
    checkInitialized = function() {};
    _triggerEvent("_init", require("filterStorage").FilterStorage.firstRun);
  }
}

Prefs._initListener = function()
{
  prefsInitDone = true;
  checkInitialized();
};

FilterNotifier.addListener(function(action)
{
  if (action === "load")
  {
    let {FilterStorage} = require("filterStorage");
    if (FilterStorage.firstRun)
    {
      // No data, must be a new user or someone with corrupted data - initialize
      // with default settings

      let {Subscription, DownloadableSubscription} = require("subscriptionClasses");
      let {Synchronizer} = require("synchronizer");
      let {Prefs} = require("prefs");
      let {Utils} = require("utils");

      // Choose default subscription and add it
      let subscriptions = require("subscriptions.xml");
      let node = Utils.chooseFilterSubscription(subscriptions);
      if (node)
      {
        let subscription = Subscription.fromURL(node.url);
        FilterStorage.addSubscription(subscription);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload)
          Synchronizer.execute(subscription);
      }
    }

    filtersInitDone = true;
    checkInitialized();
  }
});
