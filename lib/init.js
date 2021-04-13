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
const {synchronizer, addSubscriptionFilters} = require("synchronizer");
const {filterStorage} = require("filterStorage");
const {Subscription} = require("subscriptionClasses");
const {Utils} = require("utils");
const {MILLIS_IN_SECOND, MILLIS_IN_HOUR, MILLIS_IN_DAY} = require("time");

function* matchLines(text)
{
  const regex = /[^\r\n]+/g;
  let match;
  // @disable-check M19
  while (match = regex.exec(text))
    yield match;
}

function updateExpires(subscription, expires)
{
  if (!expires || expires.length < 2)
    return;

  let match = /^(\d+)\s*(h)?/.exec(expires);
  if (!match)
    return;

  let interval = parseInt(match[1], 10);
  let expirationInterval = match[2] ? interval * MILLIS_IN_HOUR :
    interval * MILLIS_IN_DAY;
  let [softExpiration, hardExpiration] =
    synchronizer._downloader.processExpirationInterval(expirationInterval);

  subscription.softExpiration = Math.round(softExpiration / MILLIS_IN_SECOND);
  subscription.expires = Math.round(hardExpiration / MILLIS_IN_SECOND);
  subscription.downloadStatus = "synchronize_ok";
  subscription.downloadCount = 0;
  subscription.errors = 0;
  // changing lastDownload required to save modifications in expires/softExpiration
  subscription.lastDownload = subscription.lastSuccess = Math.round(
    Date.now() / MILLIS_IN_SECOND
  );
}

function updateHomepage(subscription, homepage)
{
  if (!homepage)
    return;

  let url;
  try
  {
    url = new URL(homepage);
  }
  finally
  {
    if (url.protocol === "http:" || url.protocol === "https:")
      subscription.homepage = url.href;
  }
}

function updateTitle(subscription, title)
{
  if (title)
  {
    subscription.title = title;
    subscription.fixedTitle = true;
  }
  else
    subscription.fixedTitle = false;
}

function updateParams(subscription, responseText)
{
  let lines = matchLines(responseText);
  let headerMatch = /\[Adblock(?:\s*Plus\s*([\d.]+)?)?\]/i.exec(lines.next().value[0]);
  if (!headerMatch)
    return;
  let minVersion = headerMatch[1];

  let params = {
    homepage: null,
    title: null,
    expires: null
  };

  while (true)
  {
    let line = lines.next().value;
    let match = /^\s*!\s*(.*?)\s*:\s*(.*)/.exec(line[0]);
    if (!match)
      break;

    let keyword = match[1].toLowerCase();
    if (params.hasOwnProperty(keyword))
      params[keyword] = match[2];
  }

  updateExpires(subscription, params.expires);
  updateHomepage(subscription, params.homepage);
  updateTitle(subscription, params.title);
}

function injectPreload(subscription, preloadInfo)
{
  let status;
  addSubscriptionFilters(subscription, preloadInfo.content, error => { status = error; });

  if (status)
  {
    console.warn(`Failed to add preloaded subscription ${subscription.url}: ${status.error}`);
    return;
  }

  updateParams(subscription, preloadInfo.content);
}

function preload(subscription)
{
  return new Promise((resolve, reject) =>
      _resourceReader.readPreloadedFilterList(subscription.url, resolve, reject))
    .then(info =>
      {
        if (info.exists && info.content.length > 0)
          injectPreload(subscription, info);
      });
}

let registerSubscription = exports.registerSubscription = async function(subscription)
{
  filterStorage.addSubscription(subscription);
  await preload(subscription);
  if (!subscription.lastDownload && !subscription.disabled && Prefs.synchronization_enabled)
    synchronizer.execute(subscription);
}

async function startEngine()
{
  if (Prefs.first_run && Prefs.first_run_subscription_auto_select)
  {
    let node = Utils.chooseFilterSubscription([...recommendations()]);
    if (node)
    {
      let autoSubscription = Subscription.fromURL(node.url);
      autoSubscription.disabled = false;
      autoSubscription.title = node.title;
      autoSubscription.homepage = node.homepage;
      await registerSubscription(autoSubscription);
    }

    const url = Prefs.subscriptions_exceptionsurl;
    // check already configured
    if ([...filterStorage.subscriptions()].find(it => it.url === url) === undefined)
    {
      let aaSubscription = Subscription.fromURL(url);
      aaSubscription.disabled = false;
      await registerSubscription(aaSubscription);
    }
  }

  if (Prefs.synchronization_enabled)
    synchronizer.start();
  else
    synchronizer.stop();

  if (Prefs.first_run)
    Prefs.first_run = false;
}

async function initializeEngine()
{
  // This is a workaround due to the issue adblockpluscore#285. Please
  // remove when the solution of the core issue is landed in this repo.
  synchronizer._downloader.download = function(downloadable) {
    synchronizer._downloader._download(downloadable, 0);
  };

  await initializePrefs();
  await filterEngine.initialize();
  await startEngine();

  _triggerEvent("_init");
}

initializeEngine().catch(e => { throw e; });
