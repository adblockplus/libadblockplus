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

function escapeChar(chr)
{
  if (!chr || 0 === chr.length)
    return "";

  let code = chr.charCodeAt(0);

  // Control characters and leading digits must be escaped based on
  // their char code in CSS. Moreover, curly brackets aren't allowed
  // in elemhide filters, and therefore must be escaped based on their
  // char code as well.
  if (code <= 0x1F || code === 0x7F || /[\d{}]/.test(chr))
    return "\\" + code.toString(16) + " ";

  return "\\" + chr;
}

let Utils = {
  get appLocale()
  {
    return _appInfo.locale;
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

  /**
   * Escapes a token (e.g. tag, id, class or attribute) to be used in
   * CSS selectors.
   *
   * @param {string} s
   * @return {string}
   * @static
   */
  escapeCSS(s)
  {
    return s.replace(/^[\d-]|[^\w\-\u0080-\uFFFF]/g, escapeChar);
  },

  /**
   * Quotes a string to be used as attribute value in CSS selectors.
   *
   * @param {string} value
   * @return {string}
   * @static
   */
  quoteCSS(value)
  {
    return '"' + value.replace(/["\\{}\x00-\x1F\x7F]/g, escapeChar) + '"';
  }

};

exports.Utils = Utils;
