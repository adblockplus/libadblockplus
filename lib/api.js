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

let API = (() =>
{
  const {Filter} = require("filterClasses");
  const {Subscription} = require("subscriptionClasses");
  const {SpecialSubscription, DownloadableSubscription} = require("subscriptionClasses");
  const {filterStorage} = require("filterStorage");
  const {defaultMatcher} = require("matcher");
  const {elemHide} = require("elemHide");
  const {elemHideEmulation} = require("elemHideEmulation");
  const {synchronizer} = require("synchronizer");
  const {Prefs} = require("prefs");
  const {recommendations} = require("recommendations");
  const SignatureVerifier = require("rsa");
  const {parseURL} = require("url");
  const {composeFilterSuggestions} = require("compose");
  const {registerSubscription} = require("init");
  const {snippets, compileScript} = require("snippets");

  function getURLInfo(url)
  {
    // Parse the minimum URL to get a URLInfo instance.
    let urlInfo = parseURL("http://a.com/");

    try
    {
      let uri = new URI(url);

      // Note: There is currently no way to update the URLInfo object other
      // than to set the private properties directly.
      urlInfo._href = url;
      urlInfo._protocol = uri.scheme + ":";
      urlInfo._hostname = uri.asciiHost;
    }
    catch (error)
    {
      urlInfo = null;
    }

    return urlInfo;
  }

  return {
    getFilterFromText(text)
    {
      text = Filter.normalize(text);
      if (!text)
        throw "Attempted to create a filter from empty text";
      return Filter.fromText(text);
    },

    isListedFilter(filter)
    {
      for (let subscription of filterStorage.subscriptions(filter.text))
      {
        if (subscription instanceof SpecialSubscription &&
            !subscription.disabled)
          return true;
      }

      return false;
    },

    addFilterToList(filter)
    {
      filterStorage.addFilter(filter);
    },

    removeFilterFromList(filter)
    {
      filterStorage.removeFilter(filter);
    },

    getListedFilters()
    {
      let filterText = new Set();
      for (let subscription of filterStorage.subscriptions())
      {
        if (subscription instanceof SpecialSubscription)
        {
          for (let text of subscription.filterText())
            filterText.add(text);
        }
      }
      return [...filterText].map(Filter.fromText);
    },

    getSubscriptionFromUrl(url)
    {
      return Subscription.fromURL(url);
    },

    getSubscriptionsFromFilter(text)
    {
      const generator = filterStorage.subscriptions(text);
      let result = [];
      for (const value of generator) {
        result.push(value);
      }

      return result;
    },

    isListedSubscription(subscription)
    {
      return filterStorage.hasSubscription(subscription);
    },

    addSubscriptionToList(subscription)
    {
      registerSubscription(subscription).catch(e => { throw e; });
    },

    removeSubscriptionFromList(subscription)
    {
      filterStorage.removeSubscription(subscription);
    },

    updateSubscription(subscription)
    {
      if (subscription instanceof DownloadableSubscription)
        synchronizer.execute(subscription);
    },

    isSubscriptionUpdating(subscription)
    {
      return synchronizer.isExecuting(subscription.url);
    },

    getListedSubscriptions()
    {
      let subscriptions = [];
      for (let subscription of filterStorage.subscriptions())
      {
        if (!(subscription instanceof SpecialSubscription))
          subscriptions.push(subscription);
      }
      return subscriptions;
    },

    getRecommendedSubscriptions()
    {
      let result = [];
      for (let {url, title, homepage, languages} of recommendations())
      {
        let subscription = Subscription.fromURL(url);
        subscription.title = title;
        subscription.homepage = homepage;

        // This isn't normally a property of a Subscription object
        if (languages.length > 0)
          subscription.prefixes = languages.join(",");

        result.push(subscription);
      }
      return result;
    },

    isAASubscription(subscription)
    {
      return subscription.url === Prefs.subscriptions_exceptionsurl;
    },

    setAASubscriptionEnabled(enabled)
    {
      let aaSubscription = [...filterStorage.subscriptions()].find(it => API.isAASubscription(it));
      // Always keep AA subscription in list, with disabled state if requested.
      // For case when you start with disabled engine, then disable AA, then run auto-configuration
      if (!aaSubscription)
      {
        aaSubscription = Subscription.fromURL(Prefs.subscriptions_exceptionsurl);
        filterStorage.addSubscription(aaSubscription);
      }
      if (!enabled)
      {
        if (aaSubscription && !aaSubscription.disabled)
          aaSubscription.disabled = true;
        return;
      }
      if (aaSubscription.disabled)
        aaSubscription.disabled = false;
      if (!aaSubscription.lastDownload)
        synchronizer.execute(aaSubscription);
    },

    isAASubscriptionEnabled()
    {
      for (let subscription of filterStorage.subscriptions())
        if (API.isAASubscription(subscription))
          return !subscription.disabled;

      return false;
    },

    checkFilterMatch(url, contentTypeMask, documentUrl, siteKey, specificOnly)
    {
      let urlInfo = getURLInfo(url);
      if (!urlInfo)
        return null;

      let documentHost = extractHostFromURL(documentUrl);

      // Number cast to 32-bit integer then back to Number
      // before passing to the API
      // @see https://stackoverflow.com/a/11385688
      //
      // During the typecast (upcast) in JsEngine#newValue(int)
      // int -> int64_t (signed long)
      // sign bit is transfered to the new value
      // hence the value becomes negative
      // (0x80000000 -> -0x80000000 = 0x80000000 -> 0xffffffff80000000)
      // we have to convert it to unsigned before passing futher
      contentTypeMask = contentTypeMask >>> 0;

      return defaultMatcher.match(urlInfo, contentTypeMask, documentHost,
                                  siteKey, specificOnly);
    },

    getElementHidingStyleSheet(url, specificOnly)
    {
      let host = url.indexOf(':') != -1 ? extractHostFromURL(url) : url;
      return elemHide.getStyleSheet(host, specificOnly).code;
    },

    getElementHidingEmulationSelectors(url)
    {
      let host = url.indexOf(':') != -1 ? extractHostFromURL(url) : url;
      return elemHideEmulation.getFilters(host);
    },

    getPref(pref)
    {
      return Prefs[pref];
    },

    setPref(pref, value)
    {
      Prefs[pref] = value;
    },

    verifySignature(key, signature, uri, host, userAgent)
    {
      return SignatureVerifier.verifySignature(key, signature, uri + "\0" + host + "\0" + userAgent);
    },

    composeFilterSuggestions(baseUrl, tagName, id, src, style, classes, relatedUrls)
    {
      return composeFilterSuggestions(baseUrl, tagName, id, src, style, classes, relatedUrls);
    },

    startSynchronization()
    {
      synchronizer.start();
      Prefs.synchronization_enabled = true;
    },

    stopSynchronization()
    {
      synchronizer.stop();
      Prefs.synchronization_enabled = false;
    },

    getSnippetsScript(documentUrl, snippetLibrary)
    {
      let documentHost = extractHostFromURL(documentUrl);
      let scripts = snippets.getFilters(documentHost).map(it => it.script);

      if (!scripts.length)
        return "";

      return compileScript(scripts, [snippetLibrary], {});
    },
  };
})();
