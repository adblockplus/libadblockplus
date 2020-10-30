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

const {Prefs, initializePrefs} = require("prefs");
const {filterEngine} = require("filterEngine");
const {recommendations} = require("recommendations");
const {synchronizer} = require("synchronizer");

let startEngine = exports.startEngine = function()
{
  if (Prefs.first_run)
  {
    if (Prefs.first_run_subscription_auto_select)
    {
      let {filterStorage} = require("filterStorage");
      const {Subscription, DownloadableSubscription} = require("subscriptionClasses");
      const {Utils} = require("utils");

      let node = Utils.chooseFilterSubscription([...recommendations()]);
      if (node)
      {
        let subscription = Subscription.fromURL(node.url);
        subscription.disabled = false;
        subscription.title = node.title;
        subscription.homepage = node.homepage;
        filterStorage.addSubscription(subscription);

        if (subscription instanceof DownloadableSubscription && !subscription.lastDownload)
          synchronizer.execute(subscription);
      }

      const url = Prefs.subscriptions_exceptionsurl;
      // check already configured
      if ([...filterStorage.subscriptions()].find(it => it.url === url) === undefined)
      {
        let aaSubscription = Subscription.fromURL(url);
        aaSubscription.disabled = false;
        filterStorage.addSubscription(aaSubscription);

        if (aaSubscription instanceof DownloadableSubscription && !aaSubscription.lastDownload)
          synchronizer.execute(aaSubscription);
      }
    }

    Prefs.first_run = false;
  }

  if (!synchronizer._started)
    synchronizer.start();
}

function initializeEngine()
{
  let proxied = synchronizer._downloader._doCheck;

  synchronizer._downloader._doCheck = function() {
    if (Prefs.filter_engine_enabled)
      proxied.apply(this, arguments);
  };

  initializePrefs()
    .then(() => {return filterEngine.initialize();})
    .then(() => {if (Prefs.filter_engine_enabled) startEngine();})
    .then(() => _triggerEvent("_init"));
}

initializeEngine();
