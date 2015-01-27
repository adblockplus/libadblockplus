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

#ifndef ADBLOCK_PLUS_FILTER_ENGINE_H
#define ADBLOCK_PLUS_FILTER_ENGINE_H

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/Notification.h>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class FilterEngine;

  /**
   * Wrapper for an Adblock Plus filter object.
   * There are no accessors for most
   * [filter properties](https://adblockplus.org/jsdoc/adblockplus/symbols/Filter.html),
   * use `GetProperty()` to retrieve them by name.
   */
  class Filter : public JsValue,
                 public std::tr1::enable_shared_from_this<Filter>
  {
  public:
    /**
     * Filter types, see https://adblockplus.org/en/filters.
     */
    enum Type {TYPE_BLOCKING, TYPE_EXCEPTION,
               TYPE_ELEMHIDE, TYPE_ELEMHIDE_EXCEPTION,
               TYPE_COMMENT, TYPE_INVALID};

    /**
     * Retrieves the type of this filter.
     * @return Type of this filter.
     */
    Type GetType();

    /**
     * Checks whether this filter has been added to the list of custom filters.
     * @return `true` if this filter has been added.
     */
    bool IsListed();

    /**
     * Adds this filter to the list of custom filters.
     */
    void AddToList();

    /**
     * Removes this filter from the list of custom filters.
     */
    void RemoveFromList();

    bool operator==(const Filter& filter) const;

    /**
     * Creates a wrapper for an existing JavaScript filter object.
     * Normally you shouldn't call this directly, but use
     * FilterEngine::GetFilter() instead.
     * @param value JavaScript filter object.
     */
    Filter(JsValuePtr value);
  };

  /**
   * Wrapper for a subscription object.
   * There are no accessors for most
   * [subscription properties](https://adblockplus.org/jsdoc/adblockplus/symbols/Subscription.html),
   * use `GetProperty()` to retrieve them by name.
   */
  class Subscription : public JsValue,
                       public std::tr1::enable_shared_from_this<Subscription>
  {
  public:
    /**
     * Checks if this subscription has been added to the list of subscriptions.
     * @return `true` if this subscription has been added.
     */
    bool IsListed();

    /**
     * Adds this subscription to the list of subscriptions.
     */
    void AddToList();

    /**
     * Removes this subscription from the list of subscriptions.
     */
    void RemoveFromList();

    /**
     * Updates this subscription, i.e.\ retrieves the current filters from the
     * subscription URL.
     */
    void UpdateFilters();

    /**
     * Checks if the subscription is currently being updated.
     * @return `true` if the subscription is currently being updated.
     */
    bool IsUpdating();

    bool operator==(const Subscription& subscription) const;

    /**
     * Creates a wrapper for an existing JavaScript subscription object.
     * Normally you shouldn't call this directly, but use
     * FilterEngine::GetSubscription() instead.
     * @param value JavaScript subscription object.
     */
    Subscription(JsValuePtr value);
  };

  /**
   * Shared smart pointer to a `Filter` instance.
   */
  typedef std::tr1::shared_ptr<Filter> FilterPtr;

  /**
   * Shared smart pointer to a `Subscription` instance.
   */
  typedef std::tr1::shared_ptr<Subscription> SubscriptionPtr;

  /**
   * Main component of libadblockplus.
   * It handles:
   * - Filter management and matching.
   * - Subscription management and synchronization.
   * - Update checks for the application.
   */
  class FilterEngine
  {
  public:
    // Make sure to keep ContentType in sync with FilterEngine::contentTypes
    /**
     * Possible resource content types.
     */
    enum ContentType {CONTENT_TYPE_OTHER, CONTENT_TYPE_SCRIPT,
                      CONTENT_TYPE_IMAGE, CONTENT_TYPE_STYLESHEET,
                      CONTENT_TYPE_OBJECT, CONTENT_TYPE_SUBDOCUMENT,
                      CONTENT_TYPE_DOCUMENT, CONTENT_TYPE_XMLHTTPREQUEST,
                      CONTENT_TYPE_OBJECT_SUBREQUEST, CONTENT_TYPE_FONT,
                      CONTENT_TYPE_MEDIA, CONTENT_TYPE_ELEMHIDE};

    /**
     * Callback type invoked when an update becomes available.
     * The parameter is the download URL of the update.
     */
    typedef std::tr1::function<void(const std::string&)>
        UpdateAvailableCallback;

    /**
     * Callback type invoked when a manually triggered update check finishes.
     * The parameter is an optional error message.
     */
    typedef std::tr1::function<void(const std::string&)>
        UpdateCheckDoneCallback;

    /**
     * Callback type invoked when the filters change.
     * The first parameter is the action event code (see
     * [FilterNotifier.triggerListeners](https://adblockplus.org/jsdoc/adblockplus/symbols/FilterNotifier.html#.triggerListeners)
     * for the full list).
     * The second parameter is the filter/subscription object affected, if any.
     */
    typedef std::tr1::function<void(const std::string&, const JsValuePtr)> FilterChangeCallback;

    /**
     * Constructor.
     * @param jsEngine `JsEngine` instance used to run JavaScript code
     *        internally.
     */
    explicit FilterEngine(JsEnginePtr jsEngine);

    /**
     * Retrieves the `JsEngine` instance associated with this `FilterEngine`
     * instance.
     */
    JsEnginePtr GetJsEngine() const { return jsEngine; }

    /**
     * Checks if this is the first run of the application.
     * @return `true` if the application is running for the first time.
     */
    bool IsFirstRun() const;

    /**
     * Retrieves a filter object from its text representation.
     * @param text Text representation of the filter,
     *        see https://adblockplus.org/en/filters.
     * @return New `Filter` instance.
     */
    FilterPtr GetFilter(const std::string& text);

    /**
     * Retrieves a subscription object for the supplied URL.
     * @param url Subscription URL.
     * @return New `Subscription` instance.
     */
    SubscriptionPtr GetSubscription(const std::string& url);

    /**
     * Retrieves the list of custom filters.
     * @return List of custom filters.
     */
    std::vector<FilterPtr> GetListedFilters() const;

    /**
     * Retrieves all subscriptions.
     * @return List of subscriptions.
     */
    std::vector<SubscriptionPtr> GetListedSubscriptions() const;

    /**
     * Retrieves all recommended subscriptions.
     * @return List of recommended subscriptions.
     */
    std::vector<SubscriptionPtr> FetchAvailableSubscriptions() const;

    /**
     * Determines which notification is to be shown next.
     * @param url URL to match notifications to (optional).
     * @return Notification to be shown, or `null` if there is no any.
     */
    NotificationPtr GetNextNotificationToShow(
      const std::string& url = std::string());

    /**
     * Checks if any active filter matches the supplied URL.
     * @param url URL to match.
     * @param contentType Content type of the requested resource.
     * @param documentUrl URL of the document requesting the resource.
     *        Note that there will be more than one document if frames are
     *        involved, see
     *        Matches(const std::string&, const std::string&, const std::vector<std::string>&) const.
     * @return Matching filter, or `null` if there was no match.
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    FilterPtr Matches(const std::string& url,
        ContentType contentType,
        const std::string& documentUrl) const;

    /**
     * Checks if any active filter matches the supplied URL.
     * @param url URL to match.
     * @param contentType Content type of the requested resource.
     * @param documentUrls Chain of documents requesting the resource, starting
     *        with the current resource's parent frame, ending with the
     *        top-level frame.
     *        If the application is not capable of identifying the frame
     *        structure, e.g. because it is a proxy, it can be approximated
     *        using `ReferrerMapping`.
     * @return Matching filter, or a `null` if there was no match.
     * @throw `std::invalid_argument`, if an invalid `contentType` was supplied.
     */
    FilterPtr Matches(const std::string& url,
        ContentType contentType,
        const std::vector<std::string>& documentUrls) const;

    /**
     * Retrieves CSS selectors for all element hiding filters active on the
     * supplied domain.
     * @param domain Domain to retrieve CSS selectors for.
     * @return List of CSS selectors.
     */
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;

    /**
     * Retrieves a preference value.
     * @param pref Preference name.
     * @return Preference value, or `null` if it doesn't exist.
     */
    JsValuePtr GetPref(const std::string& pref) const;

    /**
     * Sets a preference value.
     * @param pref Preference name.
     * @param value New value of the preference.
     */
    void SetPref(const std::string& pref, JsValuePtr value);

    /**
     * Extracts the host from a URL.
     * @param url URL to extract the host from.
     * @return Extracted host.
     */
    std::string GetHostFromURL(const std::string& url);

    /**
     * Sets the callback invoked when an application update becomes available.
     * @param callback Callback to invoke.
     */
    void SetUpdateAvailableCallback(UpdateAvailableCallback callback);

    /**
     * Removes the callback invoked when an application update becomes
     * available.
     */
    void RemoveUpdateAvailableCallback();

    /**
     * Forces an immediate update check.
     * `FilterEngine` will automatically check for updates in regular intervals,
     * so applications should only call this when the user triggers an update
     * check manually.
     * @param callback Optional callback to invoke when the update check is
     *        finished. The string parameter will be empty when the update check
     *        succeeded, or contain an error message if it failed.
     *        Note that the callback will be invoked whether updates are
     *        available or not - to react to updates being available, use
     *        `FilterEngine::SetUpdateAvailableCallback()`.
     */
    void ForceUpdateCheck(UpdateCheckDoneCallback callback = 0);

    /**
     * Sets the callback invoked when the filters change.
     * @param callback Callback to invoke.
     */
    void SetFilterChangeCallback(FilterChangeCallback callback);

    /**
     * Removes the callback invoked when the filters change.
     */
    void RemoveFilterChangeCallback();

    /**
     * Compares two version strings in
     * [Mozilla toolkit version format](https://developer.mozilla.org/en/docs/Toolkit_version_format).
     * @param v1 First version string.
     * @param v2 Second version string.
     * @return
     *         - `0` if `v1` and `v2` are identical.
     *         - A negative number if `v1` is less than `v2`.
     *         - A positive number if `v1` is greater than `v2`.
     */
    int CompareVersions(const std::string& v1, const std::string& v2);

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

  private:
    JsEnginePtr jsEngine;
    bool initialized;
    bool firstRun;
    int updateCheckId;
    static const std::map<ContentType, std::string> contentTypes;

    void InitDone(JsValueList& params);
    FilterPtr CheckFilterMatch(const std::string& url,
                               ContentType contentType,
                               const std::string& documentUrl) const;
    void UpdateAvailable(UpdateAvailableCallback callback, JsValueList& params);
    void UpdateCheckDone(const std::string& eventName,
                         UpdateCheckDoneCallback callback, JsValueList& params);
    void FilterChanged(FilterChangeCallback callback, JsValueList& params);
  };
}

#endif
