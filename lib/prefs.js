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

//
// The values are hardcoded for now.
//

let defaults = {
  __proto__: null,
  enabled: true,
  patternsbackups: 5,
  patternsbackupinterval: 24,
  savestats: false,
  privateBrowsing: false,
  subscriptions_fallbackerrors: 5,
  subscriptions_fallbackurl: "https://adblockplus.org/getSubscription?version=%VERSION%&url=%SUBSCRIPTION%&downloadURL=%URL%&error=%ERROR%&responseStatus=%RESPONSESTATUS%",
  subscriptions_autoupdate: true,
  subscriptions_exceptionsurl: "https://easylist-downloads.adblockplus.org/exceptionrules.txt",
  documentation_link: "https://adblockplus.org/redirect?link=%LINK%&lang=%LANG%",
  currentVersion: "0.0",
  notificationdata: {},
  suppress_first_run_page: false,
  first_run_subscription_auto_select: true,
  allowed_connection_type: "",
  filter_engine_enabled: true,
  first_run: true
};

let preconfigurable = [
  "suppress_first_run_page", "filter_engine_enabled",
  "first_run_subscription_auto_select", "allowed_connection_type"];

let values;
let prefsFileName = "prefs.json";
let listeners = [];
let specificListeners = new Map();
let isDirty = false;
let isSaving = false;

function defineProperty(key)
{
  Object.defineProperty(Prefs, key,
    {
      get: () => values[key],
      set(value)
      {
        if (typeof value != typeof defaults[key])
          throw new Error("Attempt to change preference type");

        if (value == defaults[key])
          delete values[key];
        else
          values[key] = value;
        save();

        for (let listener of listeners)
          listener(key);

        let listeners_ = specificListeners.get(key);
        if (listeners_)
        {
          for (let listener of listeners_)
            listener(key);
        }
      },
      enumerable: true
    });
}

let initializePrefs = exports.initializePrefs = function()
{
  return new Promise((resolve, reject) =>
  {
    _fileSystem.read(prefsFileName, resolve, reject);
  }).then((result) =>
  {
    try
    {
      let data = JSON.parse(result.content);
      for (let key in data)
        if (key in defaults)
          values[key] = data[key];
    }
    catch (e)
    {
      console.error(e);
      console.trace();
    }
  }).catch(() =>
  {
    // prefs.json is expected to be missing, ignore file reading errors
  }).then(() =>
  {
    Prefs.initialized = true;
  });
}

function save()
{
  if (isSaving)
  {
    isDirty = true;
    return;
  }

  isDirty = false;
  isSaving = true;
  _fileSystem.write(prefsFileName, JSON.stringify(values), () =>
  {
    isSaving = false;
    if (isDirty)
      save();
  });
}

let Prefs = exports.Prefs = {
  initialized: false,

  addListener(listener)
  {
    if (listeners.indexOf(listener) < 0)
      listeners.push(listener);
  },

  removeListener(listener)
  {
    let index = listeners.indexOf(listener);
    if (index >= 0)
      listeners.splice(index, 1);
  },

  on(name, listener)
  {
    let listeners = specificListeners.get(name);
    if (!listeners)
      specificListeners.set(name, listeners = []);
    listeners.push(listener);
  },

  off(name, listener)
  {
    let listeners = specificListeners.get(name);
    if (listeners)
    {
      let index = listeners.indexOf(listener);
      if (index >= 0)
        listeners.splice(index, 1);
    }
  }
};

if (typeof _preconfiguredPrefs !== "undefined")
  // Update the default prefs with what was preconfigured
  for (let key in _preconfiguredPrefs)
    if (preconfigurable.indexOf(key) != -1)
      defaults[key] = _preconfiguredPrefs[key];

// Define defaults
for (let key in defaults)
  defineProperty(key);

// Set values of prefs based on defaults
values = Object.create(defaults);
