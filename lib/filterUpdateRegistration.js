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

const {filterNotifier} = require("filterNotifier");

let events = [
  "elemhideupdate",
  "load",
  "save",
  "filter.added",
  "filter.disabled",
  "filter.hitCount",
  "filter.lastHit",
  "filter.moved",
  "filter.removed",
  "subscription.added",
  "subscription.disabled",
  "subscription.downloading",
  "subscription.downloadStatus",
  "subscription.errors",
  "subscription.fixedTitle",
  "subscription.homepage",
  "subscription.lastCheck",
  "subscription.lastDownload",
  "subscription.removed",
  "subscription.title",
  "subscription.updated",
];

// Until we change libadblockplus API we need to listen to all the
// notification.
for (let event of events)
  filterNotifier.on(event, item => _triggerEvent("filterChange", event, item));
