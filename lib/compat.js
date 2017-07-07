/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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

var window = this;

//
// Module framework stuff
//

function require(module)
{
  return require.scopes[module];
}
require.scopes = {__proto__: null};

function importAll(module, globalObj)
{
  var exports = require(module);
  for (var key in exports)
    globalObj[key] = exports[key];
}

onShutdown = {
  done: false,
  add: function() {},
  remove: function() {}
};

//
// XPCOM emulation
//

var Components =
{
  interfaces:
  {
    nsIFile: {DIRECTORY_TYPE: 0},
    nsIFileURL: function() {},
    nsIHttpChannel: function() {},
    nsITimer: {TYPE_REPEATING_SLACK: 0},
    nsIInterfaceRequestor: null,
    nsIChannelEventSink: null
  },
  classes:
  {
    "@mozilla.org/timer;1":
    {
      createInstance: function()
      {
        return new FakeTimer();
      }
    },
    "@mozilla.org/xmlextras/xmlhttprequest;1":
    {
      createInstance: function()
      {
        return new XMLHttpRequest();
      }
    }
  },
  results: {},
  utils: {
    reportError: function(e)
    {
      console.error(e);
      console.trace();
    },
    import: function()
    {
    }
  },
  manager: null,
  ID: function()
  {
    return null;
  }
};
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

var XPCOMUtils =
{
  generateQI: function() {}
};

//
// Fake nsIFile implementation for our I/O
//

function FakeFile(path)
{
  this.path = path;
}
FakeFile.prototype =
{
  get leafName()
  {
    return this.path;
  },
  set leafName(value)
  {
    this.path = value;
  },
  append: function(path)
  {
    this.path += path;
  },
  clone: function()
  {
    return new FakeFile(this.path);
  },
  get parent()
  {
    return {create: function() {}};
  },
  normalize: function() {}
};

//
// Services.jsm module emulation
//

var Services =
{
  io: {
    newURI: function(uri)
    {
      if (!uri.length || uri[0] == "~")
        throw new Error("Invalid URI");

      /^([^:\/]*)/.test(uri);
      var scheme = RegExp.$1.toLowerCase();

      return {
        scheme: scheme,
        spec: uri,
        QueryInterface: function()
        {
          return this;
        }
      };
    },
    newFileURI: function(file)
    {
      var result = this.newURI("file:///" + file.path);
      result.file = file;
      return result;
    }
  },
  obs: {
    addObserver: function() {},
    removeObserver: function() {}
  },
  vc: {
    compare: function(v1, v2)
    {
      function parsePart(s)
      {
        if (!s)
          return parsePart("0");

        var part = {
          numA: 0,
          strB: "",
          numC: 0,
          extraD: ""
        };

        if (s === "*")
        {
          part.numA = Number.MAX_VALUE;
          return part;
        }

        var matches = s.match(/(\d*)(\D*)(\d*)(.*)/);
        part.numA = parseInt(matches[1], 10) || part.numA;
        part.strB = matches[2] || part.strB;
        part.numC = parseInt(matches[3], 10) || part.numC;
        part.extraD = matches[4] || part.extraD;

        if (part.strB == "+")
        {
          part.numA++;
          part.strB = "pre";
        }

        return part;
      }

      function comparePartElement(s1, s2)
      {
        if (s1 === "" && s2 !== "")
          return 1;
        if (s1 !== "" && s2 === "")
          return -1;
        return s1 === s2 ? 0 : (s1 > s2 ? 1 : -1);
      }

      function compareParts(p1, p2)
      {
        var result = 0;
        var elements = ["numA", "strB", "numC", "extraD"];
        elements.some(function(element)
        {
          result = comparePartElement(p1[element], p2[element]);
          return result;
        });
        return result;
      }

      var parts1 = v1.split(".");
      var parts2 = v2.split(".");
      for (var i = 0; i < Math.max(parts1.length, parts2.length); i++)
      {
        var result = compareParts(parsePart(parts1[i]), parsePart(parts2[i]));
        if (result)
          return result;
      }
      return 0;
    }
  }
}

//
// FileUtils.jsm module emulation
//

var FileUtils =
{
  PERMS_DIRECTORY: 0
};

function FakeTimer()
{
}
FakeTimer.prototype =
{
  delay: 0,
  callback: null,
  initWithCallback: function(callback, delay)
  {
    this.callback = callback;
    this.delay = delay;
    this.scheduleTimeout();
  },
  scheduleTimeout: function()
  {
    var me = this;
    setTimeout(function()
    {
      try
      {
        me.callback();
      }
      catch(e)
      {
        Cu.reportError(e);
      }
      me.scheduleTimeout();
    }, this.delay);
  }
};

//
// Fake XMLHttpRequest implementation
//

function XMLHttpRequest()
{
  this._requestHeaders = {};
  this._loadHandlers = [];
  this._errorHandlers = [];
};
XMLHttpRequest.prototype =
{
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
  _forbiddenRequestHeaders: {
    "accept-charset": true,
    "accept-encoding": true,
    "access-control-request-headers": true,
    "access-control-request-method": true,
    "connection": true,
    "content-length": true,
    "cookie": true,
    "cookie2": true,
    "date": true,
    "dnt": true,
    "expect": true,
    "host": true,
    "keep-alive": true,
    "origin": true,
    "referer": true,
    "te": true,
    "trailer": true,
    "transfer-encoding": true,
    "upgrade": true,
    "via": true,
  },
  _forbiddenRequestHeadersRe: new RegExp("^(Proxy|Sec)-", "i"),

  _isRequestHeaderAllowed: function(header)
  {
    if (this._forbiddenRequestHeaders.hasOwnProperty(header.toLowerCase()))
      return false;
    if (header.match(this._forbiddenRequestHeadersRe))
      return false;

    return true;
  },

  addEventListener: function(eventName, handler, capture)
  {
    var list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    if (list.indexOf(handler) < 0)
      list.push(handler);
  },

  removeEventListener: function(eventName, handler, capture)
  {
    var list;
    if (eventName == "load")
      list = this._loadHandlers;
    else if (eventName == "error")
      list = this._errorHandlers;
    else
      throw new Error("Event type " + eventName + " not supported");

    var index = list.indexOf(handler);
    if (index >= 0)
      list.splice(index, 1);
  },

  open: function(method, url, async, user, password)
  {
    if (method != "GET")
      throw new Error("Only GET requests are currently supported");
    if (typeof async != "undefined" && !async)
      throw new Error("Sync requests are not supported");
    if (typeof user != "undefined" || typeof password != "undefined")
      throw new Error("User authentification is not supported");
    if (this.readyState != 0)
      throw new Error("Already opened");

    this.readyState = 1;
    this._url = url;
  },

  send: function(data)
  {
    if (this.readyState != 1)
      throw new Error("XMLHttpRequest.send() is being called before XMLHttpRequest.open()");
    if (typeof data != "undefined" && data)
      throw new Error("Sending data to server is not supported");

    this.readyState = 3;

    var onGetDone = function(result)
    {
      this.channel.status = result.status;
      this.status = result.responseStatus;
      this.responseText = result.responseText;
      this._responseHeaders = result.responseHeaders;
      this.readyState = 4;

      // Notify event listeners
      const NS_OK = 0;
      var eventName = (this.channel.status == NS_OK ? "load" : "error");
      var event = {type: eventName};

      if (this["on" + eventName])
        this.onload.call(this, event);

      var list = this["_" + eventName + "Handlers"];
      for (var i = 0; i < list.length; i++)
        list[i].call(this, event);
    }.bind(this);
    // HACK (#5066): the code checking whether the connection is allowed is temporary,
    // the actual check should be in the core when we make a decision whether
    // to update a subscription with current connection or not, thus whether to
    // even construct XMLHttpRequest object or not.
    _isSubscriptionDownloadAllowed(function(isAllowed)
    {
      if (!isAllowed)
      {
        onGetDone({
          status: 0x804b000d, //NS_ERROR_CONNECTION_REFUSED;
          responseStatus: 0
        });
        return;
      }
      window._webRequest.GET(this._url, this._requestHeaders, onGetDone);
    }.bind(this));
  },

  overrideMimeType: function(mime)
  {
  },

  setRequestHeader: function(name, value)
  {
    if (this.readyState > 1)
      throw new Error("Cannot set request header after sending");

    if (this._isRequestHeaderAllowed(name))
      this._requestHeaders[name] = value;
    else
      console.warn("Attempt to set a forbidden header was denied: " + name);
  },

  getResponseHeader: function(name)
  {
    name = name.toLowerCase();
    if (!this._responseHeaders || !this._responseHeaders.hasOwnProperty(name))
      return null;
    else
      return this._responseHeaders[name];
  },

  channel:
  {
    status: -1,
    notificationCallbacks: {},
    loadFlags: 0,
    INHIBIT_CACHING: 0,
    VALIDATE_ALWAYS: 0,
    QueryInterface: function()
    {
      return this;
    }
  }
};

function _isSubscriptionDownloadAllowed(callback) {
  // It's a bit hacky, JsEngine interface which is used by FilterEngine does
  // not allow to inject an arbitrary callback, so we use triggerEvent
  // mechanism.
  // Yet one hack (#5039).
  var allowed_connection_type = require("prefs").Prefs.allowed_connection_type;
  if (allowed_connection_type == "")
    allowed_connection_type = null;
  _triggerEvent("_isSubscriptionDownloadAllowed", allowed_connection_type, callback);
}

// Polyfill Array.prototype.find
// from https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Array/find
// https://tc39.github.io/ecma262/#sec-array.prototype.find
if (!Array.prototype.find) {
  Object.defineProperty(Array.prototype, 'find', {
    value: function (predicate) {
      // 1. Let O be ? ToObject(this value).
      if (this == null) {
        throw new TypeError('"this" is null or not defined');
      }

      var o = Object(this);

      // 2. Let len be ? ToLength(? Get(O, "length")).
      var len = o.length >>> 0;

      // 3. If IsCallable(predicate) is false, throw a TypeError exception.
      if (typeof predicate !== 'function') {
        throw new TypeError('predicate must be a function');
      }

      // 4. If thisArg was supplied, let T be thisArg; else let T be undefined.
      var thisArg = arguments[1];

      // 5. Let k be 0.
      var k = 0;

      // 6. Repeat, while k < len
      while (k < len) {
        // a. Let Pk be ! ToString(k).
        // b. Let kValue be ? Get(O, Pk).
        // c. Let testResult be ToBoolean(? Call(predicate, T, "kValue, k, O")).
        // d. If testResult is true, return kValue.
        var kValue = o[k];
        if (predicate.call(thisArg, kValue, k, o)) {
          return kValue;
        }
        // e. Increase k by 1.
        k++;
      }
    }
  });
}
