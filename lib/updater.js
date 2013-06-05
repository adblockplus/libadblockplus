/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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
let updateUrl = (_appInfo.developmentBuild ? Prefs.update_url_devbuild : Prefs.update_url_release);
updateUrl = updateUrl.replace(/%NAME%/g, encodeURIComponent(_appInfo.name))
                     .replace(/%ID%/g, encodeURIComponent(_appInfo.id))
                     .replace(/%VERSION%/g, encodeURIComponent(_appInfo.version))
                     .replace(/%APP%/g, encodeURIComponent(_appInfo.platform));

const HOURS_TO_MILLIS = 60 * 60 * 1000;
const MIN_CHECK_INTERVAL = 18 * HOURS_TO_MILLIS;
const MAX_CHECK_INTERVAL = 30 * HOURS_TO_MILLIS;

const TYPE_AUTOMATIC = 0;
const TYPE_MANUAL = 1;

let checkForUpdates = exports.checkForUpdates = function checkForUpdates(forceCheck, callback)
{
  let now = Date.now();
  if (!forceCheck && now < Prefs.next_update_check)
  {
    if (Prefs.next_update_check - now > MAX_CHECK_INTERVAL)
      Prefs.next_update_check = now + MAX_CHECK_INTERVAL;

    window.setTimeout(checkForUpdates, Prefs.next_update_check - now);
    return;
  }

  Prefs.next_update_check = now + MIN_CHECK_INTERVAL +
      Math.random() * (MAX_CHECK_INTERVAL - MIN_CHECK_INTERVAL);
  if (!forceCheck)
    window.setTimeout(checkForUpdates, Prefs.next_update_check - now);

  let url = updateUrl.replace(/%TYPE%/g, forceCheck ? TYPE_MANUAL : TYPE_AUTOMATIC);
  let request = new XMLHttpRequest();
  request.open("GET", url);
  request.addEventListener("load", function()
  {
    try
    {
      let data = JSON.parse(request.responseText);
      let updateInfo = null;
      if (_appInfo.name in data)
        updateInfo = data[_appInfo.name];
      else if (_appInfo.name + "/" + _appInfo.platform in data)
        updateInfo = data[_appInfo.name + "/" + _appInfo.platform];

      if (updateInfo && "version" in updateInfo && "url" in updateInfo &&
          Services.vc.compare(updateInfo.version, _appInfo.version) > 0)
      {
        if (updateInfo.url.indexOf("https://") != 0)
          throw new Error("Invalid update URL, HTTPS is mandatory for updates");
        _triggerEvent("updateAvailable", updateInfo.url);
      }
      if (callback)
        callback(null);
    }
    catch (e)
    {
      Cu.reportError(e);
      if (callback)
        callback(e);
    }
  }, false);

  request.addEventListener("error", function()
  {
    let e = new Error("Update check failed (channel status " + request.channel.status + ")");
    Cu.reportError(e);
    if (callback)
      callback(e);
  }, false);

  request.send(null);
}

window.setTimeout(checkForUpdates, 0.1 * HOURS_TO_MILLIS);
