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

let Utils = {
  systemPrincipal: null,
  getString(id)
  {
    return id;
  },
  get appLocale()
  {
    return _appInfo.locale;
  },
  generateChecksum(lines)
  {
    // We cannot calculate MD5 checksums yet :-(
    return null;
  },

  checkLocaleLanguageMatch(languages)
  {
    for (let language of languages || [])
    {
      if (new RegExp("^" + language + "\\b").test(this.appLocale))
        return language;
    }

    return null;
  },

  chooseFilterSubscription(subscriptions)
  {
    let selectedItem = null;
    let selectedLanguage = null;
    let matchCount = 0;
    for (let i = 0; i < subscriptions.length; i++)
    {
      let subscription = subscriptions[i];
      if (!selectedItem)
        selectedItem = subscription;

      let language = this.checkLocaleLanguageMatch(subscription.languages);
      if (language)
      {
        if (!selectedLanguage || selectedLanguage.length < language.length)
        {
          selectedItem = subscription;
          selectedLanguage = language;
          matchCount = 1;
        }
        else if (selectedLanguage && selectedLanguage.length === language.length)
        {
          matchCount++;

          // If multiple items have a matching language of the same length:
          // Select one of the items randomly, probability should be the same
          // for all items. So we replace the previous match here with
          // probability 1/N (N being the number of matches).
          if (Math.random() * matchCount < 1)
          {
            selectedItem = subscription;
            selectedLanguage = language;
          }
        }
      }
    }
    return selectedItem;
  },

  logError(error)
  {
    console.error(error);
  }
};

exports.Utils = Utils;
