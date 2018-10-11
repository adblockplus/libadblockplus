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
  const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});
  const {Filter} = require("filterClasses");
  const {Subscription} = require("subscriptionClasses");
  const {SpecialSubscription} = require("subscriptionClasses");
  const {FilterStorage} = require("filterStorage");
  const {defaultMatcher} = require("matcher");
  const {ElemHide} = require("elemHide");
  const {Synchronizer} = require("synchronizer");
  const {Prefs} = require("prefs");
  const {Notification} = require("notification");

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
      return [...filter.subscriptions()]
        .some(s => (s instanceof SpecialSubscription && !s.disabled));
    },

    addFilterToList(filter)
    {
      FilterStorage.addFilter(filter);
    },

    removeFilterFromList(filter)
    {
      FilterStorage.removeFilter(filter);
    },

    getListedFilters()
    {
      let filters = {};
      for (let i = 0; i < FilterStorage.subscriptions.length; i++)
      {
        let subscription = FilterStorage.subscriptions[i];
        if (subscription instanceof SpecialSubscription)
        {
          for (let j = 0; j < subscription.filters.length; j++)
          {
            let filter = subscription.filters[j];
            if (!(filter.text in filters))
              filters[filter.text] = filter;
          }
        }
      }
      return Object.keys(filters).map(k =>
      {
        return filters[k];
      });
    },

    getSubscriptionFromUrl(url)
    {
      return Subscription.fromURL(url);
    },

    isListedSubscription(subscription)
    {
      return FilterStorage.knownSubscriptions.has(subscription.url);
    },

    addSubscriptionToList(subscription)
    {
      FilterStorage.addSubscription(subscription);

      if (!subscription.lastDownload)
        Synchronizer.execute(subscription);
    },

    removeSubscriptionFromList(subscription)
    {
      FilterStorage.removeSubscription(subscription);
    },

    updateSubscription(subscription)
    {
      Synchronizer.execute(subscription);
    },

    isSubscriptionUpdating(subscription)
    {
      return Synchronizer.isExecuting(subscription.url);
    },

    getListedSubscriptions()
    {
      return FilterStorage.subscriptions.filter(s =>
      {
        return !(s instanceof SpecialSubscription);
      });
    },

    getRecommendedSubscriptions()
    {
      let subscriptions = require("subscriptions.xml");
      let result = [];
      for (let i = 0; i < subscriptions.length; i++)
      {
        let subscription = Subscription.fromURL(subscriptions[i].url);
        subscription.title = subscriptions[i].title;
        subscription.homepage = subscriptions[i].homepage;

        // These aren't normally properties of a Subscription object
        subscription.author = subscriptions[i].author;
        subscription.prefixes = subscriptions[i].prefixes;
        subscription.specialization = subscriptions[i].specialization;
        result.push(subscription);
      }
      return result;
    },

    isAASubscription(subscription)
    {
      return subscription.url == Prefs.subscriptions_exceptionsurl;
    },

    setAASubscriptionEnabled(enabled)
    {
      let aaSubscription = FilterStorage.subscriptions.find(
        API.isAASubscription);
      if (!enabled)
      {
        if (aaSubscription && !aaSubscription.disabled)
          aaSubscription.disabled = true;
        return;
      }
      if (!aaSubscription)
      {
        aaSubscription = Subscription.fromURL(
          Prefs.subscriptions_exceptionsurl);
        FilterStorage.addSubscription(aaSubscription);
      }
      if (aaSubscription.disabled)
        aaSubscription.disabled = false;
      if (!aaSubscription.lastDownload)
        Synchronizer.execute(aaSubscription);
    },

    isAASubscriptionEnabled()
    {
      let aaSubscription = FilterStorage.subscriptions.find(
        API.isAASubscription);
      return aaSubscription && !aaSubscription.disabled;
    },

    showNextNotification(url)
    {
      Notification.showNext(url);
    },

    getNotificationTexts(notification)
    {
      return Notification.getLocalizedTexts(notification);
    },

    markNotificationAsShown(id)
    {
      Notification.markAsShown(id);
    },

    checkFilterMatch(url, contentTypeMask, documentUrl)
    {
      let requestHost = extractHostFromURL(url);
      let documentHost = extractHostFromURL(documentUrl);
      let thirdParty = isThirdParty(requestHost, documentHost);
      return defaultMatcher.matchesAny(
        url, contentTypeMask, documentHost, thirdParty);
    },

    getElementHidingSelectors(domain)
    {
      return ElemHide.getSelectorsForDomain(domain,
                                            ElemHide.ALL_MATCHING, false);
    },

    getPref(pref)
    {
      return Prefs[pref];
    },

    setPref(pref, value)
    {
      Prefs[pref] = value;
    },

    getHostFromUrl(url)
    {
      return extractHostFromURL(url);
    },

    compareVersions(v1, v2)
    {
      return Services.vc.compare(v1, v2);
    }
  };
})();
