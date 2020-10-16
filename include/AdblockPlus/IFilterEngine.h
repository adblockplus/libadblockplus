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

#ifndef ADBLOCK_PLUS_FILTER_ENGINE_H
#define ADBLOCK_PLUS_FILTER_ENGINE_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <AdblockPlus/Filter.h>
#include <AdblockPlus/IElement.h>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/Subscription.h>

namespace AdblockPlus
{
  /**
   * Main component of libadblockplus.
   * It handles:
   * - Filter management and matching.
   * - Subscription management and synchronization.
   */
  class IFilterEngine
  {
  public:
    // Make sure to keep ContentType in sync with IFilterEngine::contentTypes
    // and with URLFilter.typeMap from filterClasses.js.
    /**
     * Possible resource content types.
     */
    enum ContentType
    {
      CONTENT_TYPE_OTHER = 1,
      CONTENT_TYPE_SCRIPT = 2,
      CONTENT_TYPE_IMAGE = 4,
      CONTENT_TYPE_STYLESHEET = 8,
      CONTENT_TYPE_OBJECT = 16,
      CONTENT_TYPE_SUBDOCUMENT = 32,
      CONTENT_TYPE_WEBSOCKET = 128,
      CONTENT_TYPE_WEBRTC = 256,
      CONTENT_TYPE_PING = 1024,
      CONTENT_TYPE_XMLHTTPREQUEST = 2048,
      CONTENT_TYPE_MEDIA = 16384,
      CONTENT_TYPE_FONT = 32768,
      CONTENT_TYPE_POPUP = 1 << 24,
      CONTENT_TYPE_DOCUMENT = 1 << 26,
      CONTENT_TYPE_GENERICBLOCK = 1 << 27,
      CONTENT_TYPE_ELEMHIDE = 1 << 28,
      CONTENT_TYPE_GENERICHIDE = 1 << 29
    };

    /**
     * Bitmask of `ContentType` values.
     * The underlying type is signed 32 bit integer because it is actually used
     * in JavaScript where it is converted into 32 bit signed integer.
     */
    typedef int32_t ContentTypeMask;

    /**
     * Used in the return type of GetElementHidingEmulationSelectors
     */
    struct EmulationSelector
    {
      std::string selector;
      std::string text;
    };

    /**
     * Callback type invoked when the filters change.
     * The first parameter is the action event code (see
     * [filterNotifier.on](https://adblockplus.org/jsdoc/adblockpluscore/EventEmitter.html#on)
     * for the full list).
     * The second parameter is the filter/subscription object affected, if any.
     */
    typedef std::function<void(const std::string&, JsValue&&)> FilterChangeCallback;

    /**
     * Container of name-value pairs representing a set of preferences.
     */
    typedef std::map<std::string, AdblockPlus::JsValue> Prefs;

    virtual ~IFilterEngine() = default;

    /**
     * Retrieves the `JsEngine` instance associated with this `IFilterEngine`
     * instance.
     */
    virtual JsEngine& GetJsEngine() const = 0;

    /**
     * Checks if this is the first run of the application.
     * @return `true` if the application is running for the first time.
     */
    virtual bool IsFirstRun() const = 0;

    /**
     * Retrieves a filter object from its text representation.
     * @param text Text representation of the filter,
     *        see https://adblockplus.org/en/filters.
     * @return New `Filter` instance.
     */
    virtual Filter GetFilter(const std::string& text) const = 0;

    /**
     * Retrieves a subscription object for the supplied URL.
     * @param url Subscription URL.
     * @return New `Subscription` instance.
     */
    virtual Subscription GetSubscription(const std::string& url) const = 0;

    /**
     * Retrieves the list of custom filters.
     * @return List of custom filters.
     */
    virtual std::vector<Filter> GetListedFilters() const = 0;

    /**
     * Retrieves all subscriptions.
     * @return List of subscriptions.
     */
    virtual std::vector<Subscription> GetListedSubscriptions() const = 0;

    /**
     * Retrieves all recommended subscriptions.
     * @return List of recommended subscriptions.
     */
    virtual std::vector<Subscription> FetchAvailableSubscriptions() const = 0;

    /**
     * Ensures that the Acceptable Ads subscription is enabled or disabled.
     * @param enabled
     *   - if the value is `true`
     *     - ensure that the filter set includes an enabled AA subscription,
     *       adding it if needed and enabling it if disabled.
     *   - if the value is `false`
     *     - if an AA subscription is present, disable it.
     *     - if absent, do nothing.
     */
    virtual void SetAAEnabled(bool enabled) = 0;

    /**
     * Checks whether the Acceptable Ads subscription is enabled.
     * @return `true` if the Acceptable Ads subscription is present and enabled.
     */
    virtual bool IsAAEnabled() const = 0;

    /**
     * Retrieves the URL of the Acceptable Ads subscription, what makes the URL
     * available even if subscription is not added yet.
     * @return Returns URL of the Acceptable Ads.
     */
    virtual std::string GetAAUrl() const = 0;

    /**
     * Checks if any active filter matches the supplied URL.
     * @param url URL to match.
     * @param contentTypeMask Content type mask of the requested resource.
     * @param documentUrl URL of the document requesting the resource.
     *        Note that there will be more than one document if frames are
     *        involved, see
     *        Matches(const std::string&, const std::string&, const std::vector<std::string>&)
     * const.
     * @param siteKey
     *        Optional: public key provided by the document.
     * @param specificOnly Optional: if set to `true` then skips generic filters.
     * @return Matching filter, or an invalid filter if there was no match.
     * @see Filter::IsValid()
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    virtual Filter Matches(const std::string& url,
                           ContentTypeMask contentTypeMask,
                           const std::string& documentUrl,
                           const std::string& siteKey = "",
                           bool specificOnly = false) const = 0;

    /**
     * Checks if any active filter matches the supplied URL.
     * @param url URL to match.
     * @param contentTypeMask Content type mask of the requested resource.
     * @param documentUrls Chain of documents requesting the resource, starting
     *        with the current resource's parent frame, ending with the
     *        top-level frame.
     *        If the application is not capable of identifying the frame
     *        structure, e.g. because it is a proxy, it can be approximated
     *        using `ReferrerMapping`.
     * @param siteKey
     *        Optional: public key provided by the document.
     * @param specificOnly Optional: if set to `true` then skips generic filters.
     * @return Matching filter, or am invalid filter if there was no match.
     * @see Filter::IsValid()
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    virtual Filter Matches(const std::string& url,
                           ContentTypeMask contentTypeMask,
                           const std::vector<std::string>& documentUrls,
                           const std::string& siteKey = "",
                           bool specificOnly = false) const = 0;

    /**
     * Checks if any active genericblock filter exception matches the supplied URL.
     * @param url URL to match which is actually first parent of URL for which we
     *            want to check a $genericblock filter.
     *            Value obtained by `IsGenericblockWhitelisted()` is used later
     *            on as a `specificOnly` parameter value for `Matches()` call.
     * @param documentUrl Chain of documents requesting the resource, starting
     *        with the current URL's parent frame, ending with the
     *        top-level frame.
     *        If the application is not capable of identifying the frame
     *        structure, e.g. because it is a proxy, it can be approximated
     *        using `ReferrerMapping`.
     * @param siteKey
     *        Optional: public key provided by the document.
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    virtual bool IsGenericblockWhitelisted(const std::string& url,
                                           const std::vector<std::string>& documentUrls,
                                           const std::string& sitekey = "") const = 0;

    /**
     * Checks whether the document at the supplied URL is whitelisted.
     * @param url URL of the document.
     * @param documentUrls Chain of document URLs requesting the document,
     *        starting with the current document's parent frame, ending with
     *        the top-level frame.
     *        If the application is not capable of identifying the frame
     *        structure, e.g. because it is a proxy, it can be approximated
     *        using `ReferrerMapping`.
     * @return `true` if the URL is whitelisted.
     */
    virtual bool IsDocumentWhitelisted(const std::string& url,
                                       const std::vector<std::string>& documentUrls,
                                       const std::string& sitekey = "") const = 0;

    /**
     * Checks whether element hiding is disabled at the supplied URL.
     * @param url URL of the document.
     * @param documentUrls Chain of document URLs requesting the document,
     *        starting with the current document's parent frame, ending with
     *        the top-level frame.
     *        If the application is not capable of identifying the frame
     *        structure, e.g. because it is a proxy, it can be approximated
     *        using `ReferrerMapping`.
     * @return `true` if element hiding is whitelisted for the supplied URL.
     */
    virtual bool IsElemhideWhitelisted(const std::string& url,
                                       const std::vector<std::string>& documentUrls,
                                       const std::string& sitekey = "") const = 0;

    /**
     * Retrieves CSS style sheet for all element hiding filters active on the
     * supplied domain.
     * @param domain Domain to retrieve CSS style sheet for.
     * @param specificOnly true if generic filters should not apply.
     * @return CSS style sheet.
     */
    virtual std::string GetElementHidingStyleSheet(const std::string& domain,
                                                   bool specificOnly = false) const = 0;

    /**
     * Retrieves CSS selectors for all element hiding emulation filters active on the
     * supplied domain.
     * @param domain Domain to retrieve CSS selectors for.
     * @return List of CSS selectors along with the text property.
     */
    virtual std::vector<EmulationSelector>
    GetElementHidingEmulationSelectors(const std::string& domain) const = 0;

    /**
     * Retrieves a preference value.
     * @param pref Preference name.
     * @return Preference value, or `null` if it doesn't exist.
     */
    virtual JsValue GetPref(const std::string& pref) const = 0;

    /**
     * Sets a preference value.
     * @param pref Preference name.
     * @param value New value of the preference.
     */
    virtual void SetPref(const std::string& pref, const JsValue& value) = 0;

    /**
     * Extracts the host from a URL.
     * @param url URL to extract the host from.
     * @return Extracted host.
     */
    virtual std::string GetHostFromURL(const std::string& url) const = 0;

    /**
     * Sets the callback invoked when the filters change.
     * @param callback Callback to invoke.
     */
    virtual void SetFilterChangeCallback(const FilterChangeCallback& callback) = 0;

    /**
     * Removes the callback invoked when the filters change.
     */
    virtual void RemoveFilterChangeCallback() = 0;

    /**
     * Stores the value indicating what connection types are allowed, it is
     * passed to CreateParameters::isConnectionAllowed callback.
     * @param value Stored value. nullptr means removing of any previously
     *        stored value.
     */
    virtual void SetAllowedConnectionType(const std::string* value) = 0;

    /**
     * Retrieves previously stored allowed connection type.
     * @return Preference value, or `nullptr` if it doesn't exist.
     */
    virtual std::unique_ptr<std::string> GetAllowedConnectionType() const = 0;

    /**
     * Compares two version strings in
     * [Mozilla toolkit version
     * format](https://developer.mozilla.org/en/docs/Toolkit_version_format).
     * @param v1 First version string.
     * @param v2 Second version string.
     * @return
     *         - `0` if `v1` and `v2` are identical.
     *         - A negative number if `v1` is less than `v2`.
     *         - A positive number if `v1` is greater than `v2`.
     */
    virtual int CompareVersions(const std::string& v1, const std::string& v2) const = 0;

    /**
     * Checks whether the sitekey signature is valid for the given public key and data,
     * where data consists of (uri + "\0" + host + "\0" + userAgent).
     * @param base64 encoded public key.
     * @param base64 encoded signature.
     * @param uri signed in signature.
     * @param host signed in signature.
     * @param userAgent signed in signature.
     * @return `true` if (uri + "\0" + host + "\0" + userAgent) matches signature.
     */
    virtual bool VerifySignature(const std::string& key,
                                 const std::string& signature,
                                 const std::string& uri,
                                 const std::string& host,
                                 const std::string& userAgent) const = 0;

    /**
     * Given the details of an element, return suggested filters to block or hide that element.
     * Function will query various element's attributes to make best filters match.
     * For case when element requests several resources, img srcset for example,
     * multiple filters will be generated.
     * @param element target DOM element interface.
     * @return Suggested filters list.
     */
    virtual std::vector<std::string> ComposeFilterSuggestions(const IElement* element) const = 0;

    /**
     * Adds this subscription to the list of subscriptions.
     */
    virtual void AddSubscription(const Subscription& subscripton) = 0;

    /**
     * Removes this subscription from the list of subscriptions.
     */
    virtual void RemoveSubscription(const Subscription& subscription) = 0;

    /**
     * Adds this filter to the list of custom filters.
     */
    virtual void AddFilter(const Filter& filter) = 0;

    /**
     * Removes this filter from the list of custom filters.
     */
    virtual void RemoveFilter(const Filter& filter) = 0;

    /**
     * Retrieves the `ContentType` for the supplied string.
     * @param contentType Content type string.
     * @return The `ContentType` for the string.
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    static ContentType StringToContentType(const std::string& contentType);

    /**
     * Retrieves the string representation of the supplied `ContentType`.
     * @param contentType `ContentType` value.
     * @return The string representation of `contentType`.
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    static std::string ContentTypeToString(ContentType contentType);
  };
}

#endif
