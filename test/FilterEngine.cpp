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

#include <condition_variable>
#include <thread>

#include "FilterEngineTest.h"

using namespace AdblockPlus;

namespace
{
  // TODO(pstanek): GMOCK isn't there in libabp.
  struct FakeFilterEventObserver : IFilterEngine::EventObserver
  {
    void OnFilterEvent(IFilterEngine::FilterEvent event, const Filter& filter) override
    {
      filterEvents.push_back(event);
      lastFilter.reset(new Filter(filter));
    }

    void OnSubscriptionEvent(IFilterEngine::SubscriptionEvent event,
                             const Subscription& subscription) override
    {
      subscriptionEvents.push_back(event);
      lastSubscription.reset(new Subscription(subscription));
    }

    void Reset()
    {
      filterEvents.clear();
      subscriptionEvents.clear();
      lastFilter = nullptr;
      lastSubscription = nullptr;
    }

    std::vector<IFilterEngine::FilterEvent> filterEvents;
    std::vector<IFilterEngine::SubscriptionEvent> subscriptionEvents;
    std::unique_ptr<Filter> lastFilter;
    std::unique_ptr<Subscription> lastSubscription;
  };

   class FilterEngineIsSubscriptionDownloadAllowedTest : public BaseJsTest
  {
  protected:
    typedef std::vector<std::pair<bool, std::string>> ConnectionTypes;
    DelayedWebRequest::SharedTasks webRequestTasks;
    DelayedTimer::SharedTasks timerTasks;
    FilterEngineFactory::CreationParameters createParams;
    ConnectionTypes capturedConnectionTypes;
    bool isConnectionAllowed;
    std::vector<std::function<void(bool)>> isSubscriptionDownloadAllowedCallbacks;
    LazyFileSystem* fileSystem;
    bool isFilterEngineCreated;
    FakeFilterEventObserver observer;

    void SetUp() override
    {
      isConnectionAllowed = true;
      isFilterEngineCreated = false;

      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer = DelayedTimer::New(timerTasks);
      platformParams.fileSystem.reset(fileSystem = new LazyFileSystem());
      platformParams.webRequest = DelayedWebRequest::New(webRequestTasks);
      platformParams.resourceReader.reset(new DefaultResourceReader());
      platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(platformParams));

      createParams.preconfiguredPrefs.clear();
      createParams.preconfiguredPrefs.booleanPrefs.emplace(
          FilterEngineFactory::BooleanPrefName::FirstRunSubscriptionAutoselect, false);

      createParams.isSubscriptionDownloadAllowedCallback =
          [this](const std::string* allowedConnectionType,
                 const std::function<void(bool)>& isSubscriptionDownloadAllowedCallback) {
            capturedConnectionTypes.emplace_back(!!allowedConnectionType,
                                                 allowedConnectionType ? *allowedConnectionType
                                                                       : std::string());
            isSubscriptionDownloadAllowedCallbacks.emplace_back(
                isSubscriptionDownloadAllowedCallback);
          };
    }

    IFilterEngine& GetFilterEngine()
    {
      if (!isFilterEngineCreated)
        throw std::logic_error("Check that IFilterEngine is properly initialized");
      return platform->GetFilterEngine();
    }

    Subscription
    EnsureExampleSubscriptionAndForceUpdate(const std::string& apppendToUrl = std::string())
    {
      auto subscriptionUrl = "https://example" + apppendToUrl;
      bool isSubscriptionDownloadStatusReceived = false;
      if (!isFilterEngineCreated)
      {
        ::CreateFilterEngine(*platform, createParams);
        isFilterEngineCreated = true;
        platform->GetFilterEngine().SetFilterChangeCallback(
            [&isSubscriptionDownloadStatusReceived, &subscriptionUrl](const std::string& action,
                                                                      JsValue&& item) {
              if (action == "subscription.downloadStatus" &&
                  item.GetProperty("url").AsString() == subscriptionUrl)
                isSubscriptionDownloadStatusReceived = true;
            });
        platform->GetFilterEngine().AddEventObserver(&observer);
      }
      auto subscription = platform->GetFilterEngine().GetSubscription(subscriptionUrl);
      EXPECT_EQ(0, subscription.GetFilterCount()) << subscriptionUrl;
      EXPECT_EQ("", subscription.GetSynchronizationStatus()) << subscriptionUrl;
      EXPECT_EQ(0, subscription.GetLastDownloadAttemptTime());
      EXPECT_EQ(0, subscription.GetLastDownloadSuccessTime());
      subscription.UpdateFilters();

      // Since currently the check is called from implementation of web request
      // they have to been firstly scheduled, namely before processing of
      // 'is subscription download allowed' callbacks;
      DelayedTimer::ProcessImmediateTimers(timerTasks);

      for (const auto& isSubscriptionDownloadAllowedCallback :
           isSubscriptionDownloadAllowedCallbacks)
      {
        isSubscriptionDownloadAllowedCallback(isConnectionAllowed);
      }
      isSubscriptionDownloadAllowedCallbacks.clear();

      {
        auto ii_webRequest =
            std::find_if(webRequestTasks->begin(),
                         webRequestTasks->end(),
                         [&subscriptionUrl](const DelayedWebRequest::Task& task) -> bool {
                           return Utils::BeginsWith(task.url, subscriptionUrl);
                         });

        // if download is allowed then there should be a web request
        EXPECT_EQ(isConnectionAllowed, ii_webRequest != webRequestTasks->end());
        if (ii_webRequest != webRequestTasks->end())
        {
          ServerResponse exampleSubscriptionResponse;
          exampleSubscriptionResponse.responseStatus = 200;
          exampleSubscriptionResponse.status = IWebRequest::NS_OK;
          exampleSubscriptionResponse.responseText = "[Adblock Plus 2.0]\n||example.com";
          ii_webRequest->requestCallback(exampleSubscriptionResponse);
        }
      }
      EXPECT_TRUE(isSubscriptionDownloadStatusReceived);
      return subscription;
    }
  };

  class FilterEngineTestSiteKey : public FilterEngineTest
  {
  public:
    const std::string siteKey = "cNAQEBBQADSwAwSAJBAJRmzcpTevQqkWn6dJuX";
    const std::string uri = "/info/"
                            "Liquidit%C3%A4t.html?ses="
                            "Y3JlPTEzNTUyNDE2OTImdGNpZD13d3cuYWZmaWxpbmV0LXZlcnplaWNobmlzLmRlNTB"
                            "jNjAwNzIyNTlkNjQuNDA2MjE2MTImZmtpPTcyOTU2NiZ0YXNrPXNlYXJjaCZkb21haW49Y"
                            "WZmaWxpbmV0LXZlcnplaWNobmlzL"
                            "mRlJnM9ZGZmM2U5MTEzZGNhMWYyMWEwNDcmbGFuZ3VhZ2U9ZGUmYV9pZD0yJmtleXdvcmQ"
                            "9TGlxdWlkaXQlQzMlQTR0JnBvcz0"
                            "yJmt3cz03Jmt3c2k9OA==&token=AG06ipCV1LptGtY_"
                            "9gFnr0vBTPy4O0YTvwoTCObJ3N3ckrQCFYIA3wod2TwAjxgAIABQv5"
                            "WiAlCH8qgOUJGr9g9QmuuEG1CDnK0pUPbRrk5QhqDgkQNxP4Qqhz9xZe4";
    const std::string host = "www.affilinet-verzeichnis.de";
    const std::string userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.21 (KHTML, like "
                                  "Gecko) Chrome/25.0.1349.2 Safari/537.21";
    const std::string publicKey = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANnylWw2vLY4hUn9w06zQKbhKBfvjFUC"
                                  "sdFlb6TdQhxb9RXWXuI4t31c+o8fYOv/s8q1LGP"
                                  "ga3DE1L/tHU4LENMCAwEAAQ==";
    const std::string signature =
        "nLH8Vbc1rzmy0Q+Xg+bvm43IEO42h8rq5D9C0WCn/Y3ykgAoV4npzm7eMlqBSwZBLA/0DuuVsfTJT9MOVaurcA==";
  };
}

TEST_F(FilterEngineTest, FilterCreation)
{
  auto& filterEngine = GetFilterEngine();
  auto filter1 = filterEngine.GetFilter("foo");
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, filter1.GetType());
  auto filter2 = filterEngine.GetFilter("@@foo");
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_EXCEPTION, filter2.GetType());
  auto filter3 = filterEngine.GetFilter("example.com##foo");
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_ELEMHIDE, filter3.GetType());
  auto filter4 = filterEngine.GetFilter("example.com#@#foo");
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_ELEMHIDE_EXCEPTION, filter4.GetType());
  auto filter5 = filterEngine.GetFilter("  foo  ");
  ASSERT_EQ(filter1, filter5);
  auto filter6 = filterEngine.GetFilter("example.com#?#foo");
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_ELEMHIDE_EMULATION, filter6.GetType());
}

TEST_F(FilterEngineTest, AddRemoveFilters)
{
  auto& filterEngine = GetFilterEngine();
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  auto filter = filterEngine.GetFilter("foo");
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  filterEngine.AddFilter(filter);
  ASSERT_EQ(1u, filterEngine.GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine.GetListedFilters()[0]);
  filterEngine.AddFilter(filter);
  ASSERT_EQ(1u, filterEngine.GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine.GetListedFilters()[0]);
  filterEngine.RemoveFilter(filter);
  ASSERT_FALSE(filter.IsListed());
  filter.AddToList();
  ASSERT_TRUE(filter.IsListed());
  ASSERT_EQ(1u, filterEngine.GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine.GetListedFilters()[0]);
  filter.RemoveFromList();
  ASSERT_FALSE(filter.IsListed());
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  filterEngine.RemoveFilter(filter);
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
}

TEST_F(FilterEngineTest, AddedSubscriptionIsEnabled)
{
  auto subscription = GetFilterEngine().GetSubscription("https://foo/");
  EXPECT_FALSE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, DisablingSubscriptionDisablesItAndFiresEvent)
{
  auto subscription = GetFilterEngine().GetSubscription("https://foo/");
  int eventFiredCounter = 0;
  GetFilterEngine().SetFilterChangeCallback(
      [&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject) {
        if (eventName != "subscription.disabled" ||
            subscriptionObject.GetProperty("url").AsString() != "https://foo/")
          return;
        ++eventFiredCounter;
      });
  EXPECT_FALSE(subscription.IsDisabled());
  subscription.SetDisabled(true);
  EXPECT_EQ(1, eventFiredCounter);
  EXPECT_TRUE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, EnablingSubscriptionEnablesItAndFiresEvent)
{
  auto subscription = GetFilterEngine().GetSubscription("https://foo/");
  EXPECT_FALSE(subscription.IsDisabled());
  subscription.SetDisabled(true);
  EXPECT_TRUE(subscription.IsDisabled());

  int eventFiredCounter = 0;
  GetFilterEngine().SetFilterChangeCallback(
      [&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject) {
        if (eventName != "subscription.disabled" ||
            subscriptionObject.GetProperty("url").AsString() != "https://foo/")
          return;
        ++eventFiredCounter;
      });
  subscription.SetDisabled(false);
  EXPECT_EQ(1, eventFiredCounter);
  EXPECT_FALSE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, AddRemoveSubscriptions)
{
  auto& filterEngine = GetFilterEngine();
  size_t initialSize = filterEngine.GetListedSubscriptions().size();
  ASSERT_EQ(initialSize, filterEngine.GetListedSubscriptions().size());
  auto subscription = filterEngine.GetSubscription("https://foo/");
  ASSERT_EQ(initialSize, filterEngine.GetListedSubscriptions().size());
  filterEngine.AddSubscription(subscription);
  ASSERT_EQ(initialSize + 1, filterEngine.GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine.GetListedSubscriptions()[initialSize]);
  filterEngine.AddSubscription(subscription);
  ASSERT_EQ(initialSize + 1, filterEngine.GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine.GetListedSubscriptions()[initialSize]);
  filterEngine.RemoveSubscription(subscription);
  ASSERT_FALSE(subscription.IsListed());
  subscription.AddToList();
  ASSERT_TRUE(subscription.IsListed());
  ASSERT_EQ(initialSize + 1, filterEngine.GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine.GetListedSubscriptions()[initialSize]);
  subscription.RemoveFromList();
  ASSERT_FALSE(subscription.IsListed());
  ASSERT_EQ(initialSize, filterEngine.GetListedSubscriptions().size());
  filterEngine.RemoveSubscription(subscription);
  ASSERT_EQ(initialSize, filterEngine.GetListedSubscriptions().size());
}

TEST_F(FilterEngineTest, SubscriptionUpdates)
{
  auto subscription = GetFilterEngine().GetSubscription("https://foo/");
  ASSERT_FALSE(subscription.IsUpdating());
  subscription.UpdateFilters();
}

TEST_F(FilterEngineTest, RecommendedSubscriptions)
{
  auto& filterEngine = GetFilterEngine();
  auto subscriptions = filterEngine.FetchAvailableSubscriptions();
  auto listed = filterEngine.GetListedSubscriptions();
  EXPECT_FALSE(subscriptions.empty());

  for (const auto& cur : subscriptions)
    EXPECT_FALSE(cur.IsAA());
}

TEST_F(FilterEngineTest, RecommendedSubscriptionsLanguages)
{
  auto subscriptions = GetFilterEngine().FetchAvailableSubscriptions();
  auto it = std::find_if(subscriptions.begin(), subscriptions.end(), [](const auto& cur) {
    return cur.GetTitle() == "RuAdList+EasyList";
  });
  ASSERT_TRUE(it != subscriptions.end());
  auto languages = it->GetLanguages();
  ASSERT_EQ(2u, languages.size());
  EXPECT_EQ("ru", languages[0]);
  EXPECT_EQ("uk", languages[1]);
}

TEST_F(FilterEngineTest, RecommendedSubscriptionsLanguagesEmpty)
{
  auto subscriptions = GetFilterEngine().FetchAvailableSubscriptions();
  auto it = std::find_if(subscriptions.begin(), subscriptions.end(), [](const auto& cur) {
    return cur.GetTitle() == "EasyPrivacy";
  });
  ASSERT_TRUE(it != subscriptions.end());
  auto languages = it->GetLanguages();
  EXPECT_EQ(0u, languages.size());
}

TEST_F(FilterEngineTest, Matches)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("adbanner.gif"));
  filterEngine.AddFilter(filterEngine.GetFilter("tpbanner.gif$third-party"));
  filterEngine.AddFilter(filterEngine.GetFilter("fpbanner.gif$~third-party"));
  filterEngine.AddFilter(filterEngine.GetFilter("combanner.gif$domain=example.com"));
  filterEngine.AddFilter(filterEngine.GetFilter("orgbanner.gif$domain=~example.com"));

  AdblockPlus::Filter match1 = filterEngine.Matches(
      "http://example.org/foobar.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_FALSE(match1.IsValid());

  AdblockPlus::Filter match2 = filterEngine.Matches(
      "http://example.org/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match2.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match2.GetType());

  AdblockPlus::Filter match5 = filterEngine.Matches("http://example.org/tpbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.org/");
  ASSERT_FALSE(match5.IsValid());

  AdblockPlus::Filter match6 = filterEngine.Matches("http://example.org/fpbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.org/");
  ASSERT_TRUE(match6.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match6.GetType());

  AdblockPlus::Filter match7 = filterEngine.Matches("http://example.org/tpbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.com/");
  ASSERT_TRUE(match7.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match7.GetType());

  AdblockPlus::Filter match8 = filterEngine.Matches("http://example.org/fpbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.com/");
  ASSERT_FALSE(match8.IsValid());

  AdblockPlus::Filter match9 = filterEngine.Matches("http://example.org/combanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.com/");
  ASSERT_TRUE(match9.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match9.GetType());

  AdblockPlus::Filter match10 = filterEngine.Matches("http://example.org/combanner.gif",
                                                     AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                     "http://example.org/");
  ASSERT_FALSE(match10.IsValid());

  AdblockPlus::Filter match11 = filterEngine.Matches("http://example.org/orgbanner.gif",
                                                     AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                     "http://example.com/");
  ASSERT_FALSE(match11.IsValid());

  AdblockPlus::Filter match12 = filterEngine.Matches("http://example.org/orgbanner.gif",
                                                     AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                     "http://example.org/");
  ASSERT_TRUE(match12.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match12.GetType());
}

TEST_F(FilterEngineTest, GenericblockHierarchy)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@||example.com^$genericblock,domain=example.com"));

  EXPECT_TRUE(filterEngine.IsContentAllowlisted(
      "http://example.com/add.png",
      IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
      {"http://example.com/frame.html", "http://example.com/index.html"}));

  EXPECT_FALSE(filterEngine.IsContentAllowlisted(
      "http://example.com/add.png",
      IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
      {"http://example.com/frame.html", "http://baddomain.com/index.html"}));
}

/*
 * This test shows how genericblock filter option works:
 * Page http://testpages.adblockplus.org/en/exceptions/genericblock issues following requests:
 * 1) http://testpages.adblockplus.org/testcasefiles/genericblock/target-generic.jpg
 * 2) http://testpages.adblockplus.org/testcasefiles/genericblock/target-notgeneric.jpg
 *
 * Before genericblock filter is added
 * ("@@||testpages.adblockplus.org/en/exceptions/genericblock$genericblock") both requests are
 * blocked.
 *
 * After genericblock filter is added only 2) is blocked as there is a site-specific filter for this
 * request
 * ("/testcasefiles/genericblock/target-notgeneric.jpg$domain=testpages.adblockplus.org") and 1)
 * passes as it has only a generic filter ("/testcasefiles/genericblock/target-generic.jpg").
 */
TEST_F(FilterEngineTest, GenericblockMatch)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("/testcasefiles/genericblock/target-generic.jpg"));
  filterEngine.AddFilter(filterEngine.GetFilter(
      "/testcasefiles/genericblock/target-notgeneric.jpg$domain=testpages.adblockplus.org"));
  const std::string urlGeneric =
      "http://testpages.adblockplus.org/testcasefiles/genericblock/target-generic.jpg";
  const std::string urlNotGeneric =
      "http://testpages.adblockplus.org/testcasefiles/genericblock/target-notgeneric.jpg";
  const std::string firstParent =
      "http://testpages.adblockplus.org/en/exceptions/genericblock/frame.html";
  const AdblockPlus::IFilterEngine::ContentType contentType =
      AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE;
  const std::vector<std::string> documentUrlsForGenericBlock = {
      "http://testpages.adblockplus.org/testcasefiles/genericblock/frame.html",
      "http://testpages.adblockplus.org/en/exceptions/genericblock/"};
  const std::string immediateParent = documentUrlsForGenericBlock.front();

  bool specificOnly = filterEngine.IsContentAllowlisted(
      firstParent, IFilterEngine::CONTENT_TYPE_GENERICBLOCK, documentUrlsForGenericBlock);
  EXPECT_FALSE(specificOnly);

  AdblockPlus::Filter match1 =
      filterEngine.Matches(urlNotGeneric, contentType, immediateParent, "", specificOnly);
  ASSERT_TRUE(match1.IsValid());
  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match1.GetType());

  specificOnly = filterEngine.IsContentAllowlisted(
      firstParent, IFilterEngine::CONTENT_TYPE_GENERICBLOCK, documentUrlsForGenericBlock);
  EXPECT_FALSE(specificOnly);

  AdblockPlus::Filter match2 =
      filterEngine.Matches(urlGeneric, contentType, immediateParent, "", specificOnly);
  ASSERT_TRUE(match2.IsValid());
  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match2.GetType());

  // Now add genericblock filter and do the checks
  filterEngine.AddFilter(filterEngine.GetFilter(
      "@@||testpages.adblockplus.org/en/exceptions/genericblock$genericblock"));

  specificOnly = filterEngine.IsContentAllowlisted(
      firstParent, IFilterEngine::CONTENT_TYPE_GENERICBLOCK, documentUrlsForGenericBlock);
  EXPECT_TRUE(specificOnly);

  match1 = filterEngine.Matches(urlNotGeneric, contentType, immediateParent, "", specificOnly);
  ASSERT_TRUE(match1.IsValid()); // This is still blocked as a site-specific blocking filter applies
  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match1.GetType());

  specificOnly = filterEngine.IsContentAllowlisted(
      firstParent, IFilterEngine::CONTENT_TYPE_GENERICBLOCK, documentUrlsForGenericBlock);
  EXPECT_TRUE(specificOnly);

  match2 = filterEngine.Matches(urlGeneric, contentType, immediateParent, "", specificOnly);
  EXPECT_FALSE(match2.IsValid()); // Now with genericblock this request is not blocked
}

TEST_F(FilterEngineTest, GenericblockWithDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@||foo.example.com^$genericblock,domain=example.net"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@||bar.example.com^$genericblock,domain=~example.net"));

  EXPECT_TRUE(filterEngine.IsContentAllowlisted("http://foo.example.com/ad.html",
                                                IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
                                                {"http://foo.example.com/", "http://example.net"}));
  EXPECT_FALSE(filterEngine.IsContentAllowlisted("http://bar.example.com/ad.html",
                                                 IFilterEngine::CONTENT_TYPE_GENERICBLOCK,
                                                 {"http://bar.example.com", "http://example.net"}));
}

TEST_F(FilterEngineTest, Generichide)
{
  const char* url = "http://foo.com/bar";
  const char* docUrl = "http://foo.com/";

  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("@@bar.com$generichide"));
  AdblockPlus::Filter match1 =
      filterEngine.Matches(url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE, docUrl);
  EXPECT_FALSE(match1.IsValid());

  filterEngine.AddFilter(filterEngine.GetFilter("@@foo.com$generichide"));
  AdblockPlus::Filter match2 =
      filterEngine.Matches(url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE, docUrl);
  ASSERT_TRUE(match2.IsValid()); // should be Filter instance
  EXPECT_EQ("@@foo.com$generichide", match2.GetRaw());
}

TEST_F(FilterEngineTest, MatchesWithContentTypeMask)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("adbanner.gif.js$script,image"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@notbanner.gif"));
  filterEngine.AddFilter(filterEngine.GetFilter("blockme"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.doc^$document"));
  filterEngine.AddFilter(filterEngine.GetFilter("||popexample.com^$popup"));

  EXPECT_TRUE(
      filterEngine
          .Matches("http://popexample.com/", AdblockPlus::IFilterEngine::CONTENT_TYPE_POPUP, "")
          .IsValid());

  EXPECT_FALSE(filterEngine
                   .Matches("http://example.org/foobar.gif",
                            AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                            "")
                   .IsValid())
      << "another url should not match";

  EXPECT_FALSE(filterEngine
                   .Matches("http://example.org/adbanner.gif.js",
                            /*mask*/ 0,
                            "")
                   .IsValid())
      << "zero mask should not match (filter with some options)";

  EXPECT_FALSE(filterEngine
                   .Matches("http://example.xxx/blockme",
                            /*mask*/ 0,
                            "")
                   .IsValid())
      << "zero mask should not match (filter without any option)";

  EXPECT_FALSE(filterEngine
                   .Matches("http://example.org/adbanner.gif.js",
                            AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT,
                            "")
                   .IsValid())
      << "one arbitrary flag in mask should not match";

  EXPECT_TRUE(filterEngine
                  .Matches("http://example.org/adbanner.gif.js",
                           AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT,
                           "")
                  .IsValid())
      << "one of flags in mask should match";

  EXPECT_TRUE(filterEngine
                  .Matches("http://example.org/adbanner.gif.js",
                           AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                           "")
                  .IsValid())
      << "both flags in mask should match";

  EXPECT_TRUE(filterEngine
                  .Matches("http://example.org/adbanner.gif.js",
                           AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT |
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT,
                           "")
                  .IsValid())
      << "both flags with another flag in mask should match";

  EXPECT_TRUE(filterEngine
                  .Matches("http://example.org/adbanner.gif.js",
                           AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT |
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT,
                           "")
                  .IsValid())
      << "one of flags in mask should match";
}

TEST_F(FilterEngineTestSiteKey, SiteKeySignatureVerifier)
{
  auto& filterEngine = GetFilterEngine();
  ASSERT_FALSE(filterEngine.VerifySignature("", "", "", "", ""))
      << "should fail with empty arguments";
  ASSERT_FALSE(filterEngine.VerifySignature("publicKey", signature, uri, host, userAgent))
      << "should fail with invalid publicKey format";
  ASSERT_FALSE(filterEngine.VerifySignature(publicKey, "signature", uri, host, userAgent))
      << "should fail with invalid signature format";
  ASSERT_FALSE(filterEngine.VerifySignature(signature, publicKey, uri, host, siteKey))
      << "should fail with mixed arguments";
  ASSERT_FALSE(filterEngine.VerifySignature(publicKey, signature, host, uri, siteKey))
      << "should fail with mixed arguments";
  ASSERT_TRUE(filterEngine.VerifySignature(publicKey, signature, uri, host, userAgent));
}

TEST_F(FilterEngineTestSiteKey, MatchesBlockingSiteKey)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("/script.js$sitekey=" + siteKey));

  AdblockPlus::Filter match1 = filterEngine.Matches("http://example.org/script.js",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                                                    "http://example.org/");
  ASSERT_FALSE(match1.IsValid()) << "should not match without siteKey";

  AdblockPlus::Filter match2 = filterEngine.Matches("http://example.org/script.img",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.org/",
                                                    siteKey);
  ASSERT_FALSE(match2.IsValid()) << "should not match different content type";

  AdblockPlus::Filter match3 = filterEngine.Matches("http://example.org/script.js",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                                                    "http://example.org/",
                                                    siteKey);
  ASSERT_TRUE(match3.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match3.GetType());
}

TEST_F(FilterEngineTestSiteKey, MatchesAllowlistedSiteKey)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("adbanner.gif"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@||ads.com$image,sitekey=" + siteKey));

  AdblockPlus::Filter match1 = filterEngine.Matches("http://ads.com/adbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://example.org/",
                                                    siteKey);
  ASSERT_TRUE(match1.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_EXCEPTION, match1.GetType());

  AdblockPlus::Filter match2 = filterEngine.Matches("http://ads.com/adbanner.js",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                                                    "http://example.org/",
                                                    siteKey);
  ASSERT_FALSE(match2.IsValid()) << "should not match different content type";
}

TEST_F(FilterEngineTestSiteKey, MatchesAllowlistedSiteKeyFromNestedFrameRequest)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("adbanner.gif"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@adbanner.gif$domain=example.org,sitekey=" + siteKey));

  AdblockPlus::Filter match1 = filterEngine.Matches("http://ads.com/adbanner.gif",
                                                    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                                                    "http://ads.com/frame/",
                                                    siteKey);
  ASSERT_TRUE(match1.IsValid());
  ASSERT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, match1.GetType());
}

TEST_F(FilterEngineTestSiteKey, IsDocAndIsElemhideAllowlistedMatchesAllowlistedSiteKey)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("adframe"));
  auto docSiteKey = siteKey + "_document";
  auto elemhideSiteKey = siteKey + "_elemhide";
  filterEngine.AddFilter(filterEngine.GetFilter("@@$document,sitekey=" + docSiteKey));
  filterEngine.AddFilter(filterEngine.GetFilter("@@$elemhide,sitekey=" + elemhideSiteKey));

  {
    // normally the frame is not allowlisted
    { // no sitekey
      AdblockPlus::Filter matchResult =
          filterEngine.Matches("http://my-ads.com/adframe",
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                               "http://example.com/");
      ASSERT_TRUE(matchResult.IsValid());
      EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, matchResult.GetType());
    }
    { // random sitekey
      AdblockPlus::Filter matchResult =
          filterEngine.Matches("http://my-ads.com/adframe",
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                               "http://example.com/",
                               siteKey);
      ASSERT_TRUE(matchResult.IsValid());
      EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, matchResult.GetType());
    }
    if (false) // TODO: should be enabled during DP-235
    {          // the sitekey, but filter does not allowlist subdocument
      AdblockPlus::Filter matchResult =
          filterEngine.Matches("http://my-ads.com/adframe",
                               AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                               "http://example.com/",
                               docSiteKey);
      ASSERT_TRUE(matchResult.IsValid());
      EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, matchResult.GetType());
    }
  }

  { // the frame itself
    std::vector<std::string> documentUrls;
    documentUrls.push_back("http://example.com/");
    documentUrls.push_back("http://ads.com/");
    // no sitekey
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://my-ads.com/adframe", IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls));
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://my-ads.com/adframe", IFilterEngine::CONTENT_TYPE_ELEMHIDE, documentUrls));
    // random sitekey and the correct sitekey
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://my-ads.com/adframe", IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsContentAllowlisted("http://my-ads.com/adframe",
                                                  IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                  documentUrls,
                                                  docSiteKey));
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://my-ads.com/adframe", IFilterEngine::CONTENT_TYPE_ELEMHIDE, documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsContentAllowlisted("http://my-ads.com/adframe",
                                                  IFilterEngine::CONTENT_TYPE_ELEMHIDE,
                                                  documentUrls,
                                                  elemhideSiteKey));
  }

  { // the frame within an allowlisted frame
    std::vector<std::string> documentUrls;
    documentUrls.push_back("http://example.com/");
    documentUrls.push_back("http:/my-ads.com/adframe");
    documentUrls.push_back("http://ads.com/");
    // no sitekey
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://some-ads.com", IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls));
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://some-ads.com", IFilterEngine::CONTENT_TYPE_ELEMHIDE, documentUrls));
    // random sitekey and the correct sitekey
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://some-ads.com", IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsContentAllowlisted(
        "http://some-ads.com", IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls, docSiteKey));
    EXPECT_FALSE(filterEngine.IsContentAllowlisted(
        "http://some-ads.com", IFilterEngine::CONTENT_TYPE_ELEMHIDE, documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsContentAllowlisted("http://some-ads.com",
                                                  IFilterEngine::CONTENT_TYPE_ELEMHIDE,
                                                  documentUrls,
                                                  elemhideSiteKey));
  }
}

TEST_F(FilterEngineTest, SetRemoveFilterChangeCallback)
{
  auto& filterEngine = GetFilterEngine();
  int timesCalled = 0;
  filterEngine.SetFilterChangeCallback([&timesCalled](const std::string&, AdblockPlus::JsValue&&) {
    timesCalled++;
  });
  filterEngine.AddFilter(filterEngine.GetFilter("foo"));
  EXPECT_EQ(1, timesCalled);

  // we want to actually check the call count didn't change.
  filterEngine.RemoveFilterChangeCallback();
  filterEngine.RemoveFilter(filterEngine.GetFilter("foo"));
  EXPECT_EQ(1, timesCalled);
}

TEST_F(FilterEngineTest, AddRemoveFilterEventObserver)
{
  auto& filterEngine = GetFilterEngine();
  int timesCalled = 0;
  filterEngine.SetFilterChangeCallback([&timesCalled](const std::string&, AdblockPlus::JsValue&&) {
    timesCalled++;
  });

  std::string raw = "foo";

  FakeFilterEventObserver observer;
  filterEngine.AddEventObserver(&observer);
  filterEngine.AddFilter(filterEngine.GetFilter(raw));
  EXPECT_EQ(1, timesCalled);
  ASSERT_EQ(1u, observer.filterEvents.size());
  ASSERT_EQ(IFilterEngine::FilterEvent::FILTER_ADDED, observer.filterEvents[0]);
  ASSERT_NE(nullptr, observer.lastFilter);
  EXPECT_EQ(raw, observer.lastFilter->GetRaw());
  filterEngine.RemoveFilterChangeCallback();
  filterEngine.RemoveFilter(filterEngine.GetFilter(raw));
  EXPECT_EQ(1, timesCalled);
  ASSERT_EQ(2u, observer.filterEvents.size());
  ASSERT_EQ(IFilterEngine::FilterEvent::FILTER_REMOVED, observer.filterEvents[1]);
  ASSERT_NE(nullptr, observer.lastFilter);
  EXPECT_EQ(raw, observer.lastFilter->GetRaw());
  raw = "foo2";
  filterEngine.AddFilter(filterEngine.GetFilter(raw));
  EXPECT_EQ(1, timesCalled);
  ASSERT_EQ(3u, observer.filterEvents.size());
  ASSERT_EQ(IFilterEngine::FilterEvent::FILTER_ADDED, observer.filterEvents[2]);
  ASSERT_NE(nullptr, observer.lastFilter);
  EXPECT_EQ(raw, observer.lastFilter->GetRaw());
  filterEngine.RemoveEventObserver(&observer);
  filterEngine.RemoveFilter(filterEngine.GetFilter(raw));
  ASSERT_EQ(3u, observer.filterEvents.size());
  ASSERT_EQ(IFilterEngine::FilterEvent::FILTER_ADDED, observer.filterEvents[2]);
  ASSERT_NE(nullptr, observer.lastFilter);
  EXPECT_EQ(raw, observer.lastFilter->GetRaw());
}

TEST_F(FilterEngineTest, AddRemoveSubsciptionEventCallback)
{
  auto& filterEngine = GetFilterEngine();
  std::string url = "https://foo/";
  int timesCalled = 0;
  filterEngine.SetFilterChangeCallback([&timesCalled](const std::string&, AdblockPlus::JsValue&&) {
    timesCalled++;
  });

  FakeFilterEventObserver observer;
  filterEngine.AddEventObserver(&observer);

  auto subscription = filterEngine.GetSubscription(url);
  filterEngine.AddSubscription(subscription);
  EXPECT_EQ(2, timesCalled);
  ASSERT_EQ(2u, observer.subscriptionEvents.size());
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_ADDED, observer.subscriptionEvents[0]);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_DOWNLOADING,
            observer.subscriptionEvents[1]);
  ASSERT_NE(nullptr, observer.lastSubscription);
  EXPECT_EQ(url, observer.lastSubscription->GetUrl());

  filterEngine.RemoveFilterChangeCallback();
  observer.Reset();
  filterEngine.RemoveSubscription(subscription);
  EXPECT_EQ(2, timesCalled);
  ASSERT_EQ(1u, observer.subscriptionEvents.size());
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_REMOVED, observer.subscriptionEvents[0]);
  ASSERT_NE(nullptr, observer.lastSubscription);
  EXPECT_EQ(url, observer.lastSubscription->GetUrl());
  url = "https://bar/";
  subscription = filterEngine.GetSubscription(url);
  observer.Reset();
  filterEngine.AddSubscription(subscription);
  EXPECT_EQ(2, timesCalled);
  ASSERT_EQ(2u, observer.subscriptionEvents.size());
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_ADDED, observer.subscriptionEvents[0]);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_DOWNLOADING,
            observer.subscriptionEvents[1]);
  ASSERT_NE(nullptr, observer.lastSubscription);
  EXPECT_EQ(url, observer.lastSubscription->GetUrl());
  filterEngine.RemoveEventObserver(&observer);
  observer.Reset();
  filterEngine.RemoveSubscription(subscription);
  EXPECT_EQ(0u, observer.subscriptionEvents.size());
}

TEST_F(FilterEngineTest, DocumentAllowlisting)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.org^$document"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.com^$document,domain=example.de"));

  ASSERT_TRUE(filterEngine.IsContentAllowlisted("http://example.org/ad.html",
                                                IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                {"http://example.org/ad.html"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted("http://example.co.uk/ad.html",
                                                 IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                 {"http://example.co.uk/ad.html"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted("http://example.com/ad.html",
                                                 IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                 std::vector<std::string>()));

  ASSERT_TRUE(filterEngine.IsContentAllowlisted("http://example.com/ad.html",
                                                IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                {"http://example.com", "http://example.de"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted("http://example.co.uk/ad.html",
                                                 IFilterEngine::CONTENT_TYPE_DOCUMENT,
                                                 {"http://example.co.uk", "http://example.de"}));

  filterEngine.AddFilter(
      filterEngine.GetFilter("||testpages.adblockplus.org/testcasefiles/document/*"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@testpages.adblockplus.org/en/exceptions/document^$document"));

  // Frames hierarchy:
  // - http://testpages.adblockplus.org/en/exceptions/document
  //  - http://testpages.adblockplus.org/testcasefiles/document/frame.html
  //   - http://testpages.adblockplus.org/testcasefiles/document/image.jpg

  // Check for http://testpages.adblockplus.org/testcasefiles/document/image.jpg
  EXPECT_TRUE(filterEngine.IsContentAllowlisted(
      "http://testpages.adblockplus.org/testcasefiles/document/image.jpg",
      IFilterEngine::CONTENT_TYPE_DOCUMENT,
      {"http://testpages.adblockplus.org/testcasefiles/document/frame.html",
       "http://testpages.adblockplus.org/en/exceptions/document"}));
}

TEST_F(FilterEngineTest, ElemhideAllowlisting)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.org^$elemhide"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.com^$elemhide,domain=example.de"));

  ASSERT_TRUE(filterEngine.IsContentAllowlisted(
      "http://example.org/file", IFilterEngine::CONTENT_TYPE_ELEMHIDE, {"http://example.org"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted(
      "http://example.co.uk/file", IFilterEngine::CONTENT_TYPE_ELEMHIDE, {"http://example.co.uk"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted(
      "http://example.com/file", IFilterEngine::CONTENT_TYPE_ELEMHIDE, {"http://example.com"}));

  ASSERT_TRUE(filterEngine.IsContentAllowlisted("http://example.com/ad.html",
                                                IFilterEngine::CONTENT_TYPE_ELEMHIDE,
                                                {"http://example.com", "http://example.de"}));

  ASSERT_FALSE(filterEngine.IsContentAllowlisted("http://example.co.uk/ad.html",
                                                 IFilterEngine::CONTENT_TYPE_ELEMHIDE,
                                                 {"http://example.co.uk", "http://example.de"}));

  filterEngine.AddFilter(
      filterEngine.GetFilter("testpages.adblockplus.org##.testcase-ex-elemhide"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("@@testpages.adblockplus.org/en/exceptions/elemhide^$elemhide"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("||testpages.adblockplus.org/testcasefiles/elemhide/image.jpg"));

  // Frames hierarchy:
  // - http://testpages.adblockplus.org/en/exceptions/elemhide
  //  - http://testpages.adblockplus.org/testcasefiles/elemhide/frame.html
  //   - http://testpages.adblockplus.org/testcasefiles/elemhide/image.jpg

  std::vector<std::string> documentUrls = {
      "http://testpages.adblockplus.org/testcasefiles/elemhide/frame.html",
      "http://testpages.adblockplus.org/en/exceptions/elemhide"};

  EXPECT_TRUE(filterEngine.IsContentAllowlisted(
      "http://testpages.adblockplus.org/testcasefiles/elemhide/image.jpg",
      IFilterEngine::CONTENT_TYPE_ELEMHIDE,
      documentUrls));
  auto filter =
      filterEngine.Matches("http://testpages.adblockplus.org/testcasefiles/elemhide/image.jpg",
                           AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                           documentUrls.front());
  ASSERT_TRUE(filter.IsValid());
  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_BLOCKING, filter.GetType());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetEmpty)
{
  auto& filterEngine = GetFilterEngine();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("http://example.org");

  EXPECT_TRUE(sheet.empty());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheet)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<std::string> filters = {// other type of filters
                                      "/testcasefiles/blocking/addresspart/abptestcasepath/",
                                      "example.org#?#div:-abp-properties(width: 213px)",

                                      // element hiding selectors
                                      "###testcase-eh-id",
                                      "example.org###testcase-eh-id",
                                      "example.org##.testcase-eh-class",
                                      "example.org##.testcase-container > .testcase-eh-descendant",
                                      "~foo.example.org,example.org##foo",
                                      "~othersiteneg.org##testneg",

                                      // other site
                                      "othersite.com###testcase-eh-id"};

  for (const auto& filter : filters)
    filterEngine.AddFilter(filterEngine.GetFilter(filter));

  std::string sheet = filterEngine.GetElementHidingStyleSheet("http://example.org");

  EXPECT_EQ(
      "#testcase-eh-id {display: none !important;}\n#testcase-eh-id, .testcase-eh-class, "
      ".testcase-container > .testcase-eh-descendant, foo, testneg {display: none !important;}\n",
      sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetSingleGeneric)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors
  filterEngine.AddFilter(filterEngine.GetFilter("###testcase-eh-id"));

  std::string sheet = filterEngine.GetElementHidingStyleSheet("");

  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetSingleDomain)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors
  filterEngine.AddFilter(filterEngine.GetFilter("example.org##.testcase - eh - class"));

  std::string sheet = filterEngine.GetElementHidingStyleSheet("http://example.org");

  EXPECT_EQ(".testcase - eh - class {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetDup)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors - duplicates
  filterEngine.AddFilter(filterEngine.GetFilter("example.org###dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("example.org###dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("othersite.org###dup"));

  std::string sheet = filterEngine.GetElementHidingStyleSheet("http://example.org");

  // no dups
  EXPECT_EQ("#dup {display: none !important;}\n", sheet);

  // this makes duplicates
  filterEngine.AddFilter(filterEngine.GetFilter("~foo.example.org,example.org###dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("~bar.example.org,example.org###dup"));

  std::string sheetDup = filterEngine.GetElementHidingStyleSheet("http://example.org");

  // dups
  EXPECT_EQ("#dup, #dup, #dup {display: none !important;}\n", sheetDup);

  std::string sheetBar = filterEngine.GetElementHidingStyleSheet("http://bar.example.org");
  EXPECT_EQ("#dup, #dup {display: none !important;}\n", sheetBar);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetDiff)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("example1.org###testcase-eh-id"));
  filterEngine.AddFilter(filterEngine.GetFilter("example2.org###testcase-eh-id"));

  std::string sheet1 = filterEngine.GetElementHidingStyleSheet("http://example1.org");
  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet1);

  std::string sheet2 = filterEngine.GetElementHidingStyleSheet("http://example2.org");
  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet2);

  std::string sheetGen = filterEngine.GetElementHidingStyleSheet("");
  EXPECT_TRUE(sheetGen.empty());

  std::string sheetNonExisting = filterEngine.GetElementHidingStyleSheet("http://non-existing-domain.com");
  EXPECT_TRUE(sheetNonExisting.empty());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetGenerichide)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("##.testcase-generichide-generic"));
  filterEngine.AddFilter(filterEngine.GetFilter("example.org##.testcase-generichide-notgeneric"));
  filterEngine.AddFilter(filterEngine.GetFilter("@@||example.org$generichide"));

  std::string sheet = filterEngine.GetElementHidingStyleSheet("http://example.org");

  EXPECT_EQ(".testcase-generichide-generic {display: none "
            "!important;}\n.testcase-generichide-notgeneric {display: none !important;}\n",
            sheet);

  std::string sheetSpecificOnly = filterEngine.GetElementHidingStyleSheet("http://example.org", true);

  EXPECT_EQ(".testcase-generichide-notgeneric {display: none !important;}\n", sheetSpecificOnly);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetListJustDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("example.org###div"));
  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");
  EXPECT_EQ("#div {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetUrlPath)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("example.org###div"));
  filterEngine.AddFilter(filterEngine.GetFilter("other.org###div"));
  std::string sheet = filterEngine.GetElementHidingStyleSheet("https://example.org:123/path?q=other.org");
  EXPECT_EQ("#div {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListEmpty)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");
  EXPECT_TRUE(sels.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsAllowlist)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#foo"));

  // before allowlisting
  std::vector<IFilterEngine::EmulationSelector> selsBeforeAllowlisting =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");
  ASSERT_EQ(1u, selsBeforeAllowlisting.size());
  EXPECT_EQ("foo", selsBeforeAllowlisting[0].selector);
  EXPECT_EQ("example.org#?#foo", selsBeforeAllowlisting[0].text);

  // allowlist it
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#@#foo"));

  std::vector<IFilterEngine::EmulationSelector> selsAfterAllowlisting =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");
  EXPECT_TRUE(selsAfterAllowlisting.empty());

  // add another filter
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#another"));

  std::vector<IFilterEngine::EmulationSelector> selsAnother =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");
  ASSERT_EQ(1u, selsAnother.size());
  EXPECT_EQ("another", selsAnother[0].selector);
  EXPECT_EQ("example.org#?#another", selsAnother[0].text);

  // check another domain
  filterEngine.AddFilter(filterEngine.GetFilter("example2.org#?#foo"));

  std::vector<IFilterEngine::EmulationSelector> sels2 =
      filterEngine.GetElementHidingEmulationSelectors("http://example2.org");
  ASSERT_EQ(1u, sels2.size());
  EXPECT_EQ("foo", sels2[0].selector);
  EXPECT_EQ("example2.org#?#foo", sels2[0].text);

  // check the type of the allowlist (exception) filter
  auto filter = filterEngine.GetFilter("example.org#@#bar");
  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_ELEMHIDE_EXCEPTION, filter.GetType());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsList)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<std::string> filters = {
      // other type of filters
      "/testcasefiles/blocking/addresspart/abptestcasepath/",
      "example.org###testcase-eh-id",

      // element hiding emulation selectors
      "example.org#?#div:-abp-properties(width: 213px)",
      "example.org#?#div:-abp-has(>div>img.testcase-es-has)",
      "example.org#?#span:-abp-contains(ESContainsTarget)",
      "~foo.example.org,example.org#?#div:-abp-properties(width: 213px)",
      "~othersiteneg.org#?#div:-abp-properties(width: 213px)",

      // allowlisted
      "example.org#@#foo",

      // other site
      "othersite.com###testcase-eh-id"};

  for (const auto& filter : filters)
    filterEngine.AddFilter(filterEngine.GetFilter(filter));

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");

  ASSERT_EQ(4u, sels.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels[0].selector);
  EXPECT_EQ("div:-abp-has(>div>img.testcase-es-has)", sels[1].selector);
  EXPECT_EQ("span:-abp-contains(ESContainsTarget)", sels[2].selector);
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels[3].selector);

  // text field
  EXPECT_EQ("example.org#?#div:-abp-properties(width: 213px)", sels[0].text);
  EXPECT_EQ("example.org#?#div:-abp-has(>div>img.testcase-es-has)", sels[1].text);
  EXPECT_EQ("example.org#?#span:-abp-contains(ESContainsTarget)", sels[2].text);
  EXPECT_EQ("~foo.example.org,example.org#?#div:-abp-properties(width: 213px)", sels[3].text);

  std::vector<IFilterEngine::EmulationSelector> sels2 =
      filterEngine.GetElementHidingEmulationSelectors("http://foo.example.org");
  ASSERT_EQ(3u, sels2.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels2[0].selector);
  EXPECT_EQ("div:-abp-has(>div>img.testcase-es-has)", sels2[1].selector);
  EXPECT_EQ("span:-abp-contains(ESContainsTarget)", sels2[2].selector);

  EXPECT_EQ("example.org#?#div:-abp-properties(width: 213px)", sels2[0].text);
  EXPECT_EQ("example.org#?#div:-abp-has(>div>img.testcase-es-has)", sels2[1].text);
  EXPECT_EQ("example.org#?#span:-abp-contains(ESContainsTarget)", sels2[2].text);

  std::vector<IFilterEngine::EmulationSelector> sels3 =
      filterEngine.GetElementHidingEmulationSelectors("http://othersiteneg.org");
  ASSERT_EQ(0u, sels3.size());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListSingleDomain)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selector
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#div:-abp-properties(width: 213px)"));

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");

  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels[0].selector);
  EXPECT_EQ("example.org#?#div:-abp-properties(width: 213px)", sels[0].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListNoDuplicates)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selectors - duplicates
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("othersite.org#?#dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("~foo.example.org#?#dup"));

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");

  // no dups
  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("dup", sels[0].selector);
  EXPECT_EQ("example.org#?#dup", sels[0].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListDuplicates)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selectors - duplicates
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("~foo.example.org,example.org#?#dup"));
  filterEngine.AddFilter(filterEngine.GetFilter("~bar.example.org,example.org#?#dup"));

  std::vector<IFilterEngine::EmulationSelector> selsDups =
      filterEngine.GetElementHidingEmulationSelectors("http://example.org");

  // dups
  ASSERT_EQ(3u, selsDups.size());
  EXPECT_EQ("dup", selsDups[0].selector);
  EXPECT_EQ("dup", selsDups[1].selector);
  EXPECT_EQ("dup", selsDups[2].selector);

  EXPECT_EQ("example.org#?#dup", selsDups[0].text);
  EXPECT_EQ("~foo.example.org,example.org#?#dup", selsDups[1].text);
  EXPECT_EQ("~bar.example.org,example.org#?#dup", selsDups[2].text);

  std::vector<IFilterEngine::EmulationSelector> selsBar =
      filterEngine.GetElementHidingEmulationSelectors("http://bar.example.org");
  ASSERT_EQ(2u, selsBar.size());
  EXPECT_EQ("dup", selsBar[0].selector);
  EXPECT_EQ("dup", selsBar[1].selector);

  EXPECT_EQ("example.org#?#dup", selsBar[0].text);
  EXPECT_EQ("~foo.example.org,example.org#?#dup", selsBar[1].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListDiff)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(
      filterEngine.GetFilter("example1.org#?#div:-abp-properties(width: 213px)"));
  filterEngine.AddFilter(
      filterEngine.GetFilter("example2.org#?#div:-abp-properties(width: 213px)"));
  // allowlisted
  filterEngine.AddFilter(
      filterEngine.GetFilter("example2.org#@#div:-abp-properties(width: 213px)"));

  std::vector<IFilterEngine::EmulationSelector> sels1 =
      filterEngine.GetElementHidingEmulationSelectors("http://example1.org");
  ASSERT_EQ(1u, sels1.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels1[0].selector);
  EXPECT_EQ("example1.org#?#div:-abp-properties(width: 213px)", sels1[0].text);

  std::vector<IFilterEngine::EmulationSelector> sels2 =
      filterEngine.GetElementHidingEmulationSelectors("http://example2.org");
  ASSERT_TRUE(sels2.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsGeneric)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("example1.org#?#foo"));
  filterEngine.AddFilter(filterEngine.GetFilter("example2.org#@#bar"));

  // there are no generic el-hiding emulation filters.
  // this should have no effect on selectors returned and the type should be invalid
  auto genFilter = filterEngine.GetFilter("#?#foo");
  filterEngine.AddFilter(genFilter);

  EXPECT_EQ(AdblockPlus::Filter::Type::TYPE_INVALID, genFilter.GetType());

  std::vector<IFilterEngine::EmulationSelector> selsGen =
      filterEngine.GetElementHidingEmulationSelectors("");
  EXPECT_TRUE(selsGen.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsNonExisting)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.AddFilter(filterEngine.GetFilter("example1.org#?#foo"));
  filterEngine.AddFilter(filterEngine.GetFilter("example2.org#@#bar"));

  std::vector<IFilterEngine::EmulationSelector> selsNonExisting =
      filterEngine.GetElementHidingEmulationSelectors("http://non-existing-domain.com");
  EXPECT_TRUE(selsNonExisting.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListJustDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#div"));

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("example.org");

  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("div", sels[0].selector);
  EXPECT_EQ("example.org#?#div", sels[0].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsUrlPath)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.AddFilter(filterEngine.GetFilter("example.org#?#div"));
  filterEngine.AddFilter(filterEngine.GetFilter("other.org#?#div"));

  std::vector<IFilterEngine::EmulationSelector> sels =
      filterEngine.GetElementHidingEmulationSelectors("https://example.org:123/path?q=other.org");

  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("div", sels[0].selector);
  EXPECT_EQ("example.org#?#div", sels[0].text);
}

class TestElement : public IElement
{
public:
  TestElement(const std::map<std::string, std::string>& attributes,
              const std::vector<TestElement>& children = {})
      : data(attributes), subelemets(children)
  {
  }

  std::string GetLocalName() const override
  {
    return GetAttribute("_name");
  }

  std::string GetAttribute(const std::string& name) const override
  {
    auto it = data.find(name);
    return it == data.end() ? "" : it->second;
  }

  std::string GetDocumentLocation() const override
  {
    return GetAttribute("_url");
  }

  std::vector<const IElement*> GetChildren() const override
  {
    std::vector<const IElement*> res;
    std::transform(
        subelemets.begin(), subelemets.end(), std::back_inserter(res), [](const auto& cur) {
          return &cur;
        });
    return res;
  }

private:
  std::map<std::string, std::string> data;
  std::vector<TestElement> subelemets;
};

TEST_F(FilterEngineTest, ComposeFilterSuggestionsClass)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page"}, {"_name", "img"}, {"class", "-img   _glyph"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("test.com##.\\-img._glyph", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsAttribute)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({{"_url", "https://test.com/page"},
                       {"_name", "img"},
                       {"id", "gb_va"},
                       {"src", "data:abcd"},
                       {"style", "width:109px;height:40px"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(2u, res.size());
  EXPECT_EQ("test.com###gb_va", res[0]);
  EXPECT_EQ("test.com##img[src=\"data:abcd\"]", res[1]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsUrls)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page"},
       {"_name", "img"},
       {"id", "gb_va"},
       {"src", "https://www.static.test.com/icon1.png"},
       {"srcset",
        "https://www.static.test.com/icon1.png x1, http://test.com/ui/icon2.png x2, data:abcd"},
       {"style", "width:109px;height:40px"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);

  ASSERT_EQ(2u, res.size());
  EXPECT_EQ("||static.test.com/icon1.png", res[0]);
  EXPECT_EQ("||test.com/ui/icon2.png", res[1]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsStyle)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page"}, {"_name", "div"}, {"style", "width:109px;height:40px"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("test.com##div[style=\"width:109px;height:40px\"]", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsBaseUrl)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page/"}, {"_name", "img"}, {"src", "/icon1.png"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/icon1.png", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsIgnoreWrongProtocol)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page/"}, {"_name", "img"}, {"srcset", "data:abcd"}});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  EXPECT_EQ(0u, res.size());
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForObjectElement)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({{"_url", "https://test.com/page/"}, {"_name", "object"}},
                      {TestElement({{"_name", "param"}, {"name", "src"}, {"value", "/data1"}}),
                       TestElement({{"_name", "param"}, {"name", "src1"}, {"value", "/data2"}}),
                       TestElement({{"_name", "div"}, {"name", "src"}, {"value", "/data3"}})});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/data1", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForObjectElementData)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({{"_url", "https://test.com/page/"}, {"_name", "object"}, {"data", "data4"}},
                      {TestElement({{"_name", "param"}, {"name", "src"}, {"value", "/data1"}}),
                       TestElement({{"_name", "param"}, {"name", "src1"}, {"value", "/data2"}}),
                       TestElement({{"_name", "div"}, {"name", "src"}, {"value", "/data3"}})});

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/data4", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForMediaElement)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element(
      {{"_url", "https://test.com/page/"}, {"_name", "video"}, {"poster", "/img1.png"}},
      {
          TestElement({{"_name", "source"}, {"src", "/data1"}}),
          TestElement({{"_name", "track"}, {"src", "/data2"}}),
          TestElement({{"_name", "else"}, {"src", "/data3"}}),
      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(3u, res.size());
  EXPECT_EQ("||test.com/page/img1.png", res[0]);
  EXPECT_EQ("||test.com/page/data1", res[1]);
  EXPECT_EQ("||test.com/page/data2", res[2]);
}

TEST_F(FilterEngineWithInMemoryFS, LangAndAASubscriptionsAreChosenOnFirstRun)
{
  AppInfo appInfo;
  appInfo.locale = "zh";
  const std::string langSubscriptionUrl =
      "https://easylist-downloads.adblockplus.org/easylistchina+easylist.txt";
  InitPlatformAndAppInfo(PlatformFactory::CreationParameters(), appInfo);
  auto& filterEngine = CreateFilterEngine();
  auto subscriptions = filterEngine.GetListedSubscriptions();
  ASSERT_EQ(2u, subscriptions.size());
  Subscription* aaSubscription{nullptr};
  Subscription* langSubscription{nullptr};
  if (subscriptions[0].IsAA())
  {
    aaSubscription = &subscriptions[0];
    langSubscription = &subscriptions[1];
  }
  else if (subscriptions[1].IsAA())
  {
    aaSubscription = &(subscriptions[1]);
    langSubscription = &(subscriptions[0]);
  }
  ASSERT_NE(nullptr, aaSubscription);
  ASSERT_NE(nullptr, langSubscription);
  EXPECT_EQ(langSubscriptionUrl, langSubscription->GetUrl());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
}

TEST_F(FilterEngineWithInMemoryFS, DisableSubscriptionsAutoSelectOnFirstRun)
{
  InitPlatformAndAppInfo();
  FilterEngineFactory::CreationParameters createParams;
  createParams.preconfiguredPrefs.booleanPrefs.emplace(
      FilterEngineFactory::BooleanPrefName::FirstRunSubscriptionAutoselect, false);
  auto& filterEngine = CreateFilterEngine(createParams);
  const auto subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_EQ(0u, subscriptions.size());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
}

TEST_F(FilterEngineInitallyDisabledTest, Basic)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Enabled, EngineState::Disabled);

  EXPECT_FALSE(filterEngine.IsEnabled());
  auto init = GetJsEngine().Evaluate("require('filterStorage').filterStorage.initialized");
  EXPECT_TRUE(init.IsBool());
  EXPECT_FALSE(init.AsBool());
  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, InitallyEnabledThenDisabled)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Enabled, EngineState::Enabled);

  EXPECT_TRUE(filterEngine.IsEnabled());
  // 2 subscriptions expected because first_run_subscription_auto_select is true by default,
  // so AA and EasyList for en locale will be added.
  EXPECT_EQ(2u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(2, webRequestCounter);

  auto init = GetJsEngine().Evaluate("require(\"filterStorage\").filterStorage.initialized");
  EXPECT_TRUE(init.IsBool());
  EXPECT_FALSE(init.AsBool());

  filterEngine.SetEnabled(false);
  // webRequestCounter is not expected to increase despite requesting UpdateFilters because engine
  // is disabled
  for (auto& it : filterEngine.GetListedSubscriptions())
    it.UpdateFilters();

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_EQ(2u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(2, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, EnableWithAutoselect)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Enabled, EngineState::Disabled);

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  EXPECT_TRUE(filterEngine.IsEnabled());
  auto init = GetJsEngine().Evaluate("require('filterStorage').filterStorage.initialized");
  EXPECT_TRUE(init.IsBool());
  EXPECT_FALSE(init.AsBool());
  // 2 subscriptions expected because first_run_subscription_auto_select is true by default,
  // so AA and EasyList for en locale will be added.
  EXPECT_EQ(2u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(2, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, SubscribeWithAutoselect)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Enabled, EngineState::Disabled);

  AdblockPlus::Subscription subscription = filterEngine.GetSubscription("https://foo/");
  filterEngine.AddSubscription(subscription);

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
  EXPECT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
  // 3 subscriptions expected because first_run_subscription_auto_select is true by default,
  // so AA, EasyList for en locale and custom subscription will be added .
  EXPECT_EQ(3u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(3, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, EnableWithoutAutoselect)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Disabled, EngineState::Disabled);

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, SubscribeWithoutAutoselect)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Disabled, EngineState::Disabled);

  AdblockPlus::Subscription subscription = filterEngine.GetSubscription("https://foo/");
  filterEngine.AddSubscription(subscription);

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  EXPECT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  EXPECT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(1, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, EnableAA)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Disabled, EngineState::Disabled);

  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetAAEnabled(true);

  auto subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
  ASSERT_EQ(1u, subscriptions.size());
  EXPECT_EQ(filterEngine.GetAAUrl(), subscriptions.front().GetUrl());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
  ASSERT_EQ(1u, subscriptions.size());
  EXPECT_EQ(filterEngine.GetAAUrl(), subscriptions.front().GetUrl());
  EXPECT_EQ(1, webRequestCounter);
}

TEST_F(FilterEngineInitallyDisabledTest, DisableAA)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Enabled, EngineState::Disabled);

  auto subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_TRUE(filterEngine.IsAAEnabled());
  EXPECT_EQ(0u, subscriptions.size());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetAAEnabled(false);

  subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_FALSE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  // expect disabled AA subscription
  ASSERT_EQ(1u, subscriptions.size());
  EXPECT_EQ(filterEngine.GetAAUrl(), subscriptions.front().GetUrl());
  EXPECT_TRUE(subscriptions.front().IsDisabled());
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
  // expect disabled AA subscription and autoconfigured subscription
  ASSERT_EQ(2u, subscriptions.size());
  EXPECT_EQ(filterEngine.GetAAUrl(), subscriptions.front().GetUrl());
  EXPECT_TRUE(subscriptions.front().IsDisabled());
  // expect only autoconfigured subscription started
  EXPECT_EQ(1, webRequestCounter);
}

void ForceSynchronization(AdblockPlus::JsEngine& engine, const std::string& url)
{
  engine.Evaluate("(function(){"
                  "API.getSubscriptionFromUrl('" +
                  url +
                  "').expires = 0;"
                  "require('synchronizer').synchronizer._downloader._doCheck();"
                  "})();");
}

TEST_F(FilterEngineInitallyDisabledTest, ForceSync)
{
  auto& filterEngine = ConfigureEngine(AutoselectState::Disabled, EngineState::Disabled);

  EXPECT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(0, webRequestCounter);

  std::string url = "https://foo/";
  AdblockPlus::Subscription subscription = filterEngine.GetSubscription(url);
  filterEngine.AddSubscription(subscription);

  ForceSynchronization(GetJsEngine(), url);
  EXPECT_EQ(0, webRequestCounter);

  filterEngine.SetEnabled(true);

  EXPECT_TRUE(filterEngine.IsEnabled());
  EXPECT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  EXPECT_EQ(1, webRequestCounter);

  ForceSynchronization(GetJsEngine(), url);
  EXPECT_EQ(2, webRequestCounter);

  filterEngine.SetEnabled(false);

  ForceSynchronization(GetJsEngine(), url);
  EXPECT_EQ(2, webRequestCounter);
}

namespace AA_ApiTest
{
  const std::string kOtherSubscriptionUrl = "https://non-existing-subscription.txt";
  enum class AAStatus
  {
    absent,
    enabled,
    disabled_present
  };

  ::std::ostream& operator<<(std::ostream& os, AAStatus aaStatus)
  {
    switch (aaStatus)
    {
    case AAStatus::absent:
      os << "absent";
      break;
    case AAStatus::enabled:
      os << "enabled";
      break;
    case AAStatus::disabled_present:
      os << "disabled_present";
      break;
    default:;
    }
    return os;
  }

  enum class Action
  {
    disable,
    enable,
    remove
  };

  ::std::ostream& operator<<(std::ostream& os, Action action)
  {
    switch (action)
    {
    case Action::disable:
      os << "disable";
      break;
    case Action::enable:
      os << "enable";
      break;
    case Action::remove:
      os << "remove";
      break;
    default:;
    }
    return os;
  }

  struct Parameters
  {
    AAStatus initialAAStatus;
    Action action;
    AAStatus expectedAAStatus;
    Parameters(AAStatus aaStatus, Action action)
    {
      // if `expect` is not called then no effect is expected, initialize it now
      initialAAStatus = expectedAAStatus = aaStatus;
      this->action = action;
    }
    Parameters& expect(AAStatus aaStatus)
    {
      expectedAAStatus = aaStatus;
      return *this;
    }
    // it's merely to satisfy compiler (std::tuple requires default ctr) and
    // testing internals even calls it.
    Parameters()
    {
    }
  };

  // human readable printing for failed tests
  ::std::ostream& operator<<(::std::ostream& os, const Parameters& params)
  {
    os << "initial AA: " << params.initialAAStatus << " action: " << params.action
       << " expected AA: " << params.expectedAAStatus;
    return os;
  }
  class Test : public FilterEngineTest,
               public ::testing::WithParamInterface<
                   ::testing::tuple<Parameters, /*number of other subscriptions*/ uint8_t>>
  {
  public:
    static std::vector<Parameters> VaryPossibleCases()
    {
      // AA API test matrix
      // each column but other-subs is about AA subscription
      // enabled exists other-subs action  => expected
      //                                   => everywhere no effect on other subs
      // 1.
      // false   false  false      disable => keep with disabled flag
      // false   false  false      enable  => add and enable
      //
      // false   false  true       disable => keep with disabled flag
      // false   false  true       enable  => add and enable
      // 2.
      // false   true   false      disable => no effect
      // false   true   false      enable  => enable
      //
      // false   true   true       disable => no effect
      // false   true   true       enable  => enable
      // 3.
      // true    true   false      disable => disable
      // ture    true   false      enable  => no effect
      //
      // true    true   true       disable => disable
      // ture    true   true       enable  => no effect
      // 4.
      // false   true   false      remove  => remove
      // false   true   true       remove  => remove
      // ture    true   false      remove  => remove
      // ture    true   true       remove  => remove
      std::vector<Parameters> retValue;
      // 1.
      retValue.emplace_back(
          Parameters(AAStatus::absent, Action::disable).expect(AAStatus::disabled_present));
      retValue.emplace_back(Parameters(AAStatus::absent, Action::enable).expect(AAStatus::enabled));
      // 2.
      retValue.emplace_back(Parameters(AAStatus::disabled_present, Action::disable));
      retValue.emplace_back(
          Parameters(AAStatus::disabled_present, Action::enable).expect(AAStatus::enabled));
      // 3.
      retValue.emplace_back(
          Parameters(AAStatus::enabled, Action::disable).expect(AAStatus::disabled_present));
      retValue.emplace_back(Parameters(AAStatus::enabled, Action::enable));
      // 4.
      retValue.emplace_back(
          Parameters(AAStatus::disabled_present, Action::remove).expect(AAStatus::absent));
      retValue.emplace_back(Parameters(AAStatus::enabled, Action::remove).expect(AAStatus::absent));
      // since AA should not affect other subscriptions, the number of other
      // subscriptions is not specified here, it goes as another item in test parameters tuple.
      return retValue;
    }

    void SetUp() override
    {
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new AdblockPlus::DefaultLogSystem());
      platformParams.timer.reset(new NoopTimer());
      platformParams.fileSystem.reset(new LazyFileSystem());
      platformParams.webRequest.reset(new NoopWebRequest());
      platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(platformParams));

      FilterEngineFactory::CreationParameters params;
      params.preconfiguredPrefs.booleanPrefs.emplace(
          FilterEngineFactory::BooleanPrefName::FirstRunSubscriptionAutoselect, false);

      ::CreateFilterEngine(*platform, params);
    }

  protected:
    void init(AAStatus aaStatus, uint8_t otherSubscriptionsNumber)
    {
      // for the sake of simplicity test only with one suplimentary subscription
      ASSERT_TRUE(otherSubscriptionsNumber == 0u || otherSubscriptionsNumber == 1u);

      // this method also tests the result of intermediate steps.

      auto& filterEngine = GetFilterEngine();
      {
        // no subscription (because of preconfigured prefs.json and patterns.ini),
        // though it should be enabled by default in a non-test environment, it's tested in
        // corresponding tests.
        const auto subscriptions = filterEngine.GetListedSubscriptions();
        EXPECT_EQ(0u, subscriptions.size()); // no any subscription including AA
        EXPECT_FALSE(filterEngine.IsAAEnabled());
      }
      if (otherSubscriptionsNumber == 1u)
      {
        auto subscription = filterEngine.GetSubscription(kOtherSubscriptionUrl);
        filterEngine.AddSubscription(subscription);
        const auto subscriptions = filterEngine.GetListedSubscriptions();
        ASSERT_EQ(1u, subscriptions.size());
        EXPECT_FALSE(subscriptions[0].IsAA());
        EXPECT_EQ(kOtherSubscriptionUrl, subscriptions[0].GetUrl());
      }
      if (isAAStatusPresent(aaStatus))
      {
        filterEngine.SetAAEnabled(true); // add AA by enabling it
        if (aaStatus == AAStatus::disabled_present)
        {
          filterEngine.SetAAEnabled(false);
        }
        testSubscriptionState(aaStatus, otherSubscriptionsNumber);
      }
    }
    bool isAAStatusPresent(AAStatus aaStatus)
    {
      return aaStatus != AAStatus::absent;
    }
    void testSubscriptionState(AAStatus aaStatus, int otherSubscriptionsNumber)
    {
      auto& filterEngine = GetFilterEngine();
      if (aaStatus == AAStatus::enabled)
        EXPECT_TRUE(filterEngine.IsAAEnabled());
      else
        EXPECT_FALSE(filterEngine.IsAAEnabled());

      Subscription* aaSubscription{nullptr};
      Subscription* otherSubscription{nullptr};
      auto subscriptions = filterEngine.GetListedSubscriptions();
      for (auto& subscription : subscriptions)
      {
        if (subscription.IsAA())
          aaSubscription = &subscription;
        else
          otherSubscription = &subscription;
      }

      if (aaStatus == AAStatus::enabled)
        EXPECT_FALSE(aaSubscription->IsDisabled());
      else if (aaSubscription)
        EXPECT_TRUE(aaSubscription->IsDisabled());

      if (otherSubscriptionsNumber == 1u)
      {
        switch (aaStatus)
        {
        case AAStatus::absent:
          EXPECT_EQ(1u, subscriptions.size());
          EXPECT_EQ(nullptr, aaSubscription);
          EXPECT_NE(nullptr, otherSubscription);
          break;
        case AAStatus::enabled:
          EXPECT_EQ(2u, subscriptions.size());
          EXPECT_NE(nullptr, aaSubscription);
          EXPECT_NE(nullptr, otherSubscription);
          break;
        case AAStatus::disabled_present:
          EXPECT_EQ(2u, subscriptions.size());
          EXPECT_NE(nullptr, aaSubscription);
          EXPECT_NE(nullptr, otherSubscription);
          break;
        }
      }
      else if (otherSubscriptionsNumber == 0u)
      {
        switch (aaStatus)
        {
        case AAStatus::absent:
          EXPECT_EQ(0u, subscriptions.size());
          EXPECT_EQ(nullptr, aaSubscription);
          EXPECT_EQ(nullptr, otherSubscription);
          break;
        case AAStatus::enabled:
          EXPECT_EQ(1u, subscriptions.size());
          EXPECT_NE(nullptr, aaSubscription);
          EXPECT_EQ(nullptr, otherSubscription);
          break;
        case AAStatus::disabled_present:
          EXPECT_EQ(1u, subscriptions.size());
          EXPECT_NE(nullptr, aaSubscription);
          EXPECT_EQ(nullptr, otherSubscription);
          break;
        }
      }
    }
  };

  INSTANTIATE_TEST_SUITE_P(AA_ApiTests,
                           Test,
                           ::testing::Combine(::testing::ValuesIn(Test::VaryPossibleCases()),
                                              ::testing::Values<uint8_t>(0, 1)));

  TEST_P(Test, VaryPossibleCases)
  {
    const auto parameter = ::testing::get<0>(GetParam());
    uint8_t otherSubscriptionsNumber = ::testing::get<1>(GetParam());
    init(parameter.initialAAStatus, otherSubscriptionsNumber);
    auto& filterEngine = GetFilterEngine();

    if (parameter.action == Action::enable)
      filterEngine.SetAAEnabled(true);
    else if (parameter.action == Action::disable)
      filterEngine.SetAAEnabled(false);
    else if (parameter.action == Action::remove)
    {
      Subscription* aaSubscription{nullptr};
      auto subscriptions = filterEngine.GetListedSubscriptions();
      for (auto& subscription : subscriptions)
      {
        if (subscription.IsAA())
        {
          aaSubscription = &subscription;
          break;
        }
      }
      ASSERT_TRUE(aaSubscription != nullptr);
      filterEngine.RemoveSubscription(*aaSubscription);
    }
    testSubscriptionState(parameter.expectedAAStatus, otherSubscriptionsNumber);
  }
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, AbsentCallbackAllowsUpdating)
{
  createParams.isSubscriptionDownloadAllowedCallback =
      FilterEngineFactory::IsConnectionAllowedAsyncCallback();
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_LT(0, subscription.GetLastDownloadAttemptTime());
  EXPECT_LT(0, subscription.GetLastDownloadSuccessTime());

  EXPECT_EQ(1, subscription.GetFilterCount());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, SubscriptionEventsWhenUpdating)
{
  EnsureExampleSubscriptionAndForceUpdate("");
  ASSERT_GE(observer.subscriptionEvents.size(), 4u);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_DOWNLOADING,
            observer.subscriptionEvents[0]);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_LASTDOWNLOAD,
            observer.subscriptionEvents[1]);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_DOWNLOADSTATUS,
            observer.subscriptionEvents[2]);
  EXPECT_EQ(IFilterEngine::SubscriptionEvent::SUBSCRIPTION_UPDATED, observer.subscriptionEvents[3]);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, AllowingCallbackAllowsUpdating)
{
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_LT(0, subscription.GetLastDownloadAttemptTime());
  EXPECT_LT(0, subscription.GetLastDownloadSuccessTime());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_FALSE(capturedConnectionTypes[0].first);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, NotAllowingCallbackDoesNotAllowUpdating)
{
  isConnectionAllowed = false;
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_connection_error", subscription.GetSynchronizationStatus());
  EXPECT_LT(0, subscription.GetLastDownloadAttemptTime());
  EXPECT_EQ(0, subscription.GetLastDownloadSuccessTime());
  EXPECT_EQ(0, subscription.GetFilterCount());
  EXPECT_EQ(1u, capturedConnectionTypes.size());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest,
       PredefinedAllowedConnectionTypeIsPassedToCallback)
{
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.stringPrefs.insert(std::make_pair(
      FilterEngineFactory::StringPrefName::AllowedConnectionType, predefinedAllowedConnectionType));
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_LT(0, subscription.GetLastDownloadAttemptTime());
  EXPECT_LT(0, subscription.GetLastDownloadSuccessTime());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest,
       ConfiguredConnectionTypeIsPassedToCallbackNonMetered)
{
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.stringPrefs.insert(std::make_pair(
      FilterEngineFactory::StringPrefName::AllowedConnectionType, predefinedAllowedConnectionType));
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest,
       ConfiguredConnectionTypeIsPassedToCallbackNoValue)
{
  std::string testConnection = "";
  createParams.preconfiguredPrefs.stringPrefs.insert(
      {FilterEngineFactory::StringPrefName::AllowedConnectionType, testConnection});
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_FALSE(capturedConnectionTypes[0].first);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest,
       ConfiguredConnectionTypeIsPassedToCallbackOther)
{
  std::string testConnection = "test connection";
  createParams.preconfiguredPrefs.stringPrefs.insert(
      {FilterEngineFactory::StringPrefName::AllowedConnectionType, testConnection});
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(testConnection, capturedConnectionTypes[0].second);
}
