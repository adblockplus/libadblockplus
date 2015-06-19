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
let {Downloader, Downloadable, MILLIS_IN_HOUR} = require("downloader");

let updateUrl = (_appInfo.developmentBuild ? Prefs.update_url_devbuild : Prefs.update_url_release);
updateUrl = updateUrl.replace(/%NAME%/g, encodeURIComponent(_appInfo.name));

let callback = null;

const INITIAL_DELAY = 0.1 * MILLIS_IN_HOUR;
const CHECK_INTERVAL = 1 * MILLIS_IN_HOUR;
const EXPIRATION_INTERVAL = 24 * MILLIS_IN_HOUR;
const TYPE_AUTOMATIC = 0;
const TYPE_MANUAL = 1;

let downloader = new Downloader(getDownloadables, INITIAL_DELAY, CHECK_INTERVAL);
downloader.onExpirationChange = onExpirationChange;
downloader.onDownloadSuccess = onDownloadSuccess;
downloader.onDownloadError = onDownloadError;

function getDownloadable(forceCheck)
{
  if (!forceCheck && Pref.disable_auto_updates)
  {
    return null;
  }
  let url = updateUrl.replace(/%TYPE%/g, forceCheck ? TYPE_MANUAL : TYPE_AUTOMATIC);
  let downloadable = new Downloadable(url);
  downloadable.lastError = Prefs.update_last_error;
  downloadable.lastCheck = Prefs.update_last_check;
  downloadable.softExpiration = Prefs.update_soft_expiration;
  downloadable.hardExpiration = Prefs.update_hard_expiration;
  return downloadable;
}

function getDownloadables()
{
  yield getDownloadable(false);
}

function onExpirationChange(downloadable)
{
  Prefs.update_last_check = downloadable.lastCheck;
  Prefs.update_soft_expiration = downloadable.softExpiration;
  Prefs.update_hard_expiration = downloadable.hardExpiration;
}

function onDownloadSuccess(downloadable, responseText, errorCallback, redirectCallback)
{
  Prefs.update_last_error = 0;
  [Prefs.update_soft_expiration, Prefs.update_hard_expiration] = downloader.processExpirationInterval(EXPIRATION_INTERVAL);

  try
  {
    let data = JSON.parse(responseText);
    let updateInfo = null;
    if (_appInfo.name in data)
      updateInfo = data[_appInfo.name];
    else if (_appInfo.name + "/" + _appInfo.application in data)
      updateInfo = data[_appInfo.name + "/" + _appInfo.application];

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
    errorCallback(e);
  }

  callback = null;
}

function onDownloadError(downloadable, downloadURL, error, channelStatus, responseStatus, redirectCallback)
{
  Prefs.update_last_error = Date.now();
  if (callback)
    callback(error);
  callback = null;
}

let checkForUpdates = exports.checkForUpdates = function checkForUpdates(_callback)
{
  callback = _callback;
  downloader.download(getDownloadable(true));
}
