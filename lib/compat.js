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

let window = this;

//
// Module framework stuff
//

function require(module)
{
  // Following Issue 5762 we use relative path for require
  // https://issues.adblockplus.org/ticket/5762
  if (module.startsWith("./"))
    module = module.substring(2);
  return require.scopes[module];
}
require.scopes = {__proto__: null};

//
// Fake XMLHttpRequest implementation
//

function XMLHttpRequest()
{
  this._requestHeaders = {};
  this._loadHandlers = [];
  this._errorHandlers = [];
}
XMLHttpRequest.prototype =
{
  _method: null,
  _url: null,
  _requestHeaders: null,
  _responseHeaders: null,
  _loadHandlers: null,
  _errorHandlers: null,
  onload: null,
  onerror: null,
  status: 0,
  readyState: 0,
  responseText: null,

  // list taken from https://developer.mozilla.org/en-US/docs/Glossary/Forbidden_header_name
  _forbiddenRequestHeaders: new Set([
    "accept-charset",
    "accept-encoding",
    "access-control-request-headers",
    "access-control-request-method",
    "connection",
    "content-length",
    "cookie",
    "cookie2",
    "date",
    "dnt",
    "expect",
    "host",
    "keep-alive",
    "origin",
    "referer",
    "te",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "via"
  ]),
  _forbiddenRequestHeadersRe: new RegExp("^(Proxy|Sec)-", "i"),

  _isRequestHeaderAllowed(header)
  {
    if (this._forbiddenRequestHeaders.has(header.toLowerCase()))
      return false;
    if (header.match(this._forbiddenRequestHeadersRe))
      return false;

    return true;
  },

  _doWebRequest(method, url, requestHeaders, onRequestDone)
  {
    if (method == "GET")
      window._webRequest.GET(url, requestHeaders, onRequestDone);
    else if (method == "HEAD")
      window._webRequest.HEAD(url, requestHeaders, onRequestDone);
    else
      throw new Error("Web request method " + method + " not supported");
  },

  addEventListener(eventName, handler, capture)
  {
    let list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    if (list.indexOf(handler) < 0)
      list.push(handler);
  },

  removeEventListener(eventName, handler, capture)
  {
    let list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    let index = list.indexOf(handler);
    if (index >= 0)
      list.splice(index, 1);
  },

  open(method, url, async, user, password)
  {
    if (method != "GET" && method != "HEAD")
      throw new Error("Only GET and HEAD requests are currently supported");
    if (typeof async != "undefined" && !async)
      throw new Error("Sync requests are not supported");
    if (typeof user != "undefined" || typeof password != "undefined")
      throw new Error("User authentication is not supported");
    if (this.readyState != 0)
      throw new Error("Already opened");

    this.readyState = 1;
    this._url = url;
    this._method = method;
  },

  _fail(onRequestDone)
  {
    onRequestDone({
      status: 0x804b000d, // NS_ERROR_CONNECTION_REFUSED;
      responseStatus: 0
    });
  },

  send(data)
  {
    if (this.readyState != 1)
      throw new Error(
        "XMLHttpRequest.send() is being called before XMLHttpRequest.open()");
    if (typeof data != "undefined" && data)
      throw new Error("Sending data to server is not supported");

    this.readyState = 3;

    let onRequestDone = result =>
    {
      this.status = result.responseStatus;
      this.responseText = result.responseText;
      this._responseHeaders = result.responseHeaders;
      this.readyState = 4;

      // Notify event listeners
      const NS_OK = 0;
      let eventName = (result.status == NS_OK ? "load" : "error");
      let event = {type: eventName};

      if (this["on" + eventName])
        this["on" + eventName].call(this, event);

      let list = this["_" + eventName + "Handlers"];
      for (let i = 0; i < list.length; i++)
        list[i].call(this, event);
    };

    // #1319: Now IFilterEngine and Updater are separated, so we want to
    // allow update requests no matter if subscriptions download requests
    // are allowed or not.
    if (this._url.includes("update.json"))
    {
      this._doWebRequest(this._method, this._url, this._requestHeaders, onRequestDone);
      return;
    }

    // HACK (#5066): the code checking whether the connection is
    // allowed is temporary, the actual check should be in the core
    // when we make a decision whether to update a subscription with
    // current connection or not, thus whether to even construct
    // XMLHttpRequest object or not.
    _isSubscriptionDownloadAllowed(isAllowed =>
    {
      if (!isAllowed)
      {
        this._fail(onRequestDone);
        return;
      }
      this._doWebRequest(this._method, this._url, this._requestHeaders, onRequestDone);
    });
  },

  overrideMimeType(mime)
  {
  },

  setRequestHeader(name, value)
  {
    if (this.readyState > 1)
      throw new Error("Cannot set request header after sending");

    if (this._isRequestHeaderAllowed(name))
      this._requestHeaders[name] = value;
    else
      console.warn("Attempt to set a forbidden header was denied: " + name);
  },

  getResponseHeader(name)
  {
    name = name.toLowerCase();
    if (!this._responseHeaders || !this._responseHeaders.hasOwnProperty(name))
      return null;
    return this._responseHeaders[name];
  },

  getAllResponseHeaders()
  {
    return this._responseHeaders;
  }
};

//
// Fake URL implementation
//

class URL
{
  constructor(url, baseUrl)
  {
    // This is used by the Downloader class in adblockpluscore only to validate
    // the URL.
    // https://gitlab.com/eyeo/adblockplus/adblockpluscore/-/blob/eaae9e06f92d25b1d01bbeb19d3faf4019ef4ebc/lib/downloader.js#L251
    this.href = url;

    if (baseUrl && !url.includes(':'))
    {
      // Remove trailing slashes, to avoid something like base///realtive
      this.href = baseUrl.replace(/\/+$/, '') + '/' + url.replace(/^\/+/, '');
    }

    // Note: Unlike the real URL constructor, this may throw an error if the
    // URL is not normalized. This implementation is used for the URLs of
    // filter list subscriptions and notification feeds only. They are expected
    // to be normalized. If an error is thrown here, the Downloader class will
    // treat the URL as an invalid URL.
    //
    // The regular expression below comes from parseURL() in adblockpluscore
    // https://issues.adblockplus.org/ticket/7296
    let [, protocol, hostname] =
      /^([^:]+:)(?:\/\/(?:[^/]*@)?(\[[^\]]*\]|[^:/]+))?/.exec(this.href);

    this.protocol = protocol;
    this.hostname = hostname;
  }
}

//
// Fake fetch() implementation
//

function fetch(url, initObj)
{
  // adblockpluscore switched from XMLHttpRequest to fetch() in
  // https://issues.adblockplus.org/ticket/7381
  // This is a thin wrapper around XMLHttpRequest preserving exactly the API
  // and semantics required.
  return new Promise((resolve, reject) =>
  {
    let request = null;
    let headers = new Map();

    let handleLoad = () =>
    {
      let {status, responseText} = request;
      let responseHeaders = request.getAllResponseHeaders();

      for (const header in responseHeaders) {
        if (responseHeaders.hasOwnProperty(header)) {
            headers.set(header, responseHeaders[header]);
        }
      }

      let response = {
        status,
        text: () => Promise.resolve(responseText),
        headers
      };

      resolve(response);
    };

    try
    {
      request = new XMLHttpRequest();
      request.open(initObj.method, url);
    }
    catch (error)
    {
      reject(error);
      return;
    }

    request.addEventListener("error", reject, false);
    request.addEventListener("load", handleLoad, false);

    request.send(null);
  });
}

function _isSubscriptionDownloadAllowed(callback)
{
  // It's a bit hacky, JsEngine interface which is used by IFilterEngine does
  // not allow to inject an arbitrary callback, so we use triggerEvent
  // mechanism.
  // Yet one hack (#5039).
  let allowedConnectionType = require("prefs").Prefs.allowed_connection_type;
  if (allowedConnectionType == "")
    allowedConnectionType = null;
  _triggerEvent("_isSubscriptionDownloadAllowed", allowedConnectionType,
                callback);
}

/**
 * Fix for jsbn.js
 */
const navigator = {
  appName: ""
};

/**
 * Code taken from https://github.com/jsdom/abab/blob/master/lib/atob.js (W3C 3-clause BSD License)
 *
 * A lookup table for atob(), which converts an ASCII character to the
 * corresponding six-bit number.
 */
const keystr =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

function atobLookup(chr) {
  const index = keystr.indexOf(chr);
  return index < 0 ? undefined : index;
}

/**
 * Code taken from https://github.com/jsdom/abab/blob/master/lib/atob.js (W3C 3-clause BSD License)
 *
 * Implementation of atob() according to the HTML and Infra specs, except that
 * instead of throwing INVALID_CHARACTER_ERR we return null.
 */
function atob(data) {
  // Web IDL requires DOMStrings to just be converted using ECMAScript
  // ToString, which in our case amounts to using a template literal.
  data = `${data}`;
  // "Remove all ASCII whitespace from data."
  data = data.replace(/[ \t\n\f\r]/g, "");
  // "If data's length divides by 4 leaving no remainder, then: if data ends
  // with one or two U+003D (=) code points, then remove them from data."
  if (data.length % 4 === 0) {
    data = data.replace(/==?$/, "");
  }
  // "If data's length divides by 4 leaving a remainder of 1, then return
  // failure."
  //
  // "If data contains a code point that is not one of
  //
  // U+002B (+)
  // U+002F (/)
  // ASCII alphanumeric
  //
  // then return failure."
  if (data.length % 4 === 1 || /[^+/0-9A-Za-z]/.test(data)) {
    return null;
  }
  // "Let output be an empty byte sequence."
  let output = "";
  // "Let buffer be an empty buffer that can have bits appended to it."
  //
  // We append bits via left-shift and or.  accumulatedBits is used to track
  // when we've gotten to 24 bits.
  let buffer = 0;
  let accumulatedBits = 0;
  // "Let position be a position variable for data, initially pointing at the
  // start of data."
  //
  // "While position does not point past the end of data:"
  for (let i = 0; i < data.length; i++) {
    // "Find the code point pointed to by position in the second column of
    // Table 1: The Base 64 Alphabet of RFC 4648. Let n be the number given in
    // the first cell of the same row.
    //
    // "Append to buffer the six bits corresponding to n, most significant bit
    // first."
    //
    // atobLookup() implements the table from RFC 4648.
    buffer <<= 6;
    buffer |= atobLookup(data[i]);
    accumulatedBits += 6;
    // "If buffer has accumulated 24 bits, interpret them as three 8-bit
    // big-endian numbers. Append three bytes with values equal to those
    // numbers to output, in the same order, and then empty buffer."
    if (accumulatedBits === 24) {
      output += String.fromCharCode((buffer & 0xff0000) >> 16);
      output += String.fromCharCode((buffer & 0xff00) >> 8);
      output += String.fromCharCode(buffer & 0xff);
      buffer = accumulatedBits = 0;
    }
    // "Advance position by 1."
  }
  // "If buffer is not empty, it contains either 12 or 18 bits. If it contains
  // 12 bits, then discard the last four and interpret the remaining eight as
  // an 8-bit big-endian number. If it contains 18 bits, then discard the last
  // two and interpret the remaining 16 as two 8-bit big-endian numbers. Append
  // the one or two bytes with values equal to those one or two numbers to
  // output, in the same order."
  if (accumulatedBits === 12) {
    buffer >>= 4;
    output += String.fromCharCode(buffer);
  } else if (accumulatedBits === 18) {
    buffer >>= 2;
    output += String.fromCharCode((buffer & 0xff00) >> 8);
    output += String.fromCharCode(buffer & 0xff);
  }
  // "Return output."
  return output;
}
