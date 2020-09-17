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

#include "BaseJsTest.h"
#include <AdblockPlus/DefaultLogSystem.h>
#include <thread>
#include <condition_variable>

using namespace AdblockPlus;

namespace AdblockPlus
{
  namespace Utils
  {
    inline bool BeginsWith(const std::string& str, const std::string& beginning)
    {
      return 0 == str.compare(0, beginning.size(), beginning);
    }
  }
}

namespace
{
  class NoFilesFileSystem : public LazyFileSystem
  {
  public:
    void Stat(const std::string& /*fileName*/, const StatCallback& callback) const override
    {
      scheduler([callback]
      {
        callback(StatResult(), "");
      });
    }
  };

  template<class LazyFileSystemT, class LogSystem>
  class FilterEngineTestGeneric : public BaseJsTest
  {
  public:
  protected:
    void SetUp() override
    {
      LazyFileSystemT* fileSystem;
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LogSystem());
      platformParams.timer.reset(new NoopTimer());
      platformParams.fileSystem.reset(fileSystem = new LazyFileSystemT());
      platformParams.webRequest.reset(new NoopWebRequest());
      platform.reset(new Platform(std::move(platformParams)));
      ::CreateFilterEngine(*fileSystem, *platform);
    }

    IFilterEngine& GetFilterEngine()
    {
      return platform->GetFilterEngine();
    }
  };

  typedef FilterEngineTestGeneric<LazyFileSystem, AdblockPlus::DefaultLogSystem> FilterEngineTest;
  typedef FilterEngineTestGeneric<NoFilesFileSystem, LazyLogSystem> FilterEngineTestNoData;

  class FilterEngineWithInMemoryFS : public BaseJsTest
  {
    LazyFileSystem* fileSystem;
  protected:
    void InitPlatformAndAppInfo(const AppInfo& appInfo = AppInfo())
    {
      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer.reset(new NoopTimer());
      platformParams.fileSystem.reset(fileSystem = new InMemoryFileSystem());
      platformParams.webRequest.reset(new NoopWebRequest());
      platform.reset(new Platform(std::move(platformParams)));
      platform->SetUpJsEngine(appInfo);
    }

    IFilterEngine& CreateFilterEngine(const FilterEngineFactory::CreationParameters& creationParams = FilterEngineFactory::CreationParameters())
    {
      ::CreateFilterEngine(*fileSystem, *platform, creationParams);
      return platform->GetFilterEngine();
    }
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

    void SetUp() override
    {
      isConnectionAllowed = true;
      isFilterEngineCreated = false;

      ThrowingPlatformCreationParameters platformParams;
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer = DelayedTimer::New(timerTasks);
      platformParams.fileSystem.reset(fileSystem = new LazyFileSystem());
      platformParams.webRequest = DelayedWebRequest::New(webRequestTasks);
      platform.reset(new Platform(std::move(platformParams)));

      createParams.preconfiguredPrefs.clear();
      createParams.preconfiguredPrefs.emplace("first_run_subscription_auto_select", GetJsEngine().NewValue(false));

      createParams.isSubscriptionDownloadAllowedCallback = [this](const std::string* allowedConnectionType,
        const std::function<void(bool)>& isSubscriptionDownloadAllowedCallback){
        capturedConnectionTypes.emplace_back(!!allowedConnectionType, allowedConnectionType ? *allowedConnectionType : std::string());
        isSubscriptionDownloadAllowedCallbacks.emplace_back(isSubscriptionDownloadAllowedCallback);
      };
    }

    IFilterEngine& GetFilterEngine()
    {
      if (!isFilterEngineCreated)
        throw std::logic_error("Check that IFilterEngine is properly initialized");
      return platform->GetFilterEngine();
    }

    Subscription EnsureExampleSubscriptionAndForceUpdate(const std::string& apppendToUrl = std::string())
    {
      auto subscriptionUrl = "https://example" + apppendToUrl;
      bool isSubscriptionDownloadStatusReceived = false;
      if (!isFilterEngineCreated)
      {
        ::CreateFilterEngine(*fileSystem, *platform, createParams);
        isFilterEngineCreated = true;
        platform->GetFilterEngine().SetFilterChangeCallback([&isSubscriptionDownloadStatusReceived, &subscriptionUrl](const std::string& action, JsValue&& item)
        {
          if (action == "subscription.downloadStatus" && item.GetProperty("url").AsString() == subscriptionUrl)
            isSubscriptionDownloadStatusReceived = true;
        });
      }
      auto subscription = platform->GetFilterEngine().GetSubscription(subscriptionUrl);
      EXPECT_EQ(0, subscription.GetFilterCount()) << subscriptionUrl;
      EXPECT_EQ("", subscription.GetSynchronizationStatus()) << subscriptionUrl;
      subscription.UpdateFilters();

      // Since currently the check is called from implemenation of web request
      // they have to been firstly scheduled, namely before processing of
      // 'is subscription download allowed' callbacks;
      DelayedTimer::ProcessImmediateTimers(timerTasks);

      for (const auto& isSubscriptionDownloadAllowedCallback : isSubscriptionDownloadAllowedCallbacks)
      {
        isSubscriptionDownloadAllowedCallback(isConnectionAllowed);
      }
      isSubscriptionDownloadAllowedCallbacks.clear();

      {
        auto ii_webRequest = std::find_if(webRequestTasks->begin(), webRequestTasks->end(), [&subscriptionUrl](const DelayedWebRequest::Task& task)->bool
        {
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
          ii_webRequest->getCallback(exampleSubscriptionResponse);
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
    const std::string uri = "/info/Liquidit%C3%A4t.html?ses=Y3JlPTEzNTUyNDE2OTImdGNpZD13d3cuYWZmaWxpbmV0LXZlcnplaWNobmlzLmRlNTB"
                            "jNjAwNzIyNTlkNjQuNDA2MjE2MTImZmtpPTcyOTU2NiZ0YXNrPXNlYXJjaCZkb21haW49YWZmaWxpbmV0LXZlcnplaWNobmlzL"
                            "mRlJnM9ZGZmM2U5MTEzZGNhMWYyMWEwNDcmbGFuZ3VhZ2U9ZGUmYV9pZD0yJmtleXdvcmQ9TGlxdWlkaXQlQzMlQTR0JnBvcz0"
                            "yJmt3cz03Jmt3c2k9OA==&token=AG06ipCV1LptGtY_9gFnr0vBTPy4O0YTvwoTCObJ3N3ckrQCFYIA3wod2TwAjxgAIABQv5"
                            "WiAlCH8qgOUJGr9g9QmuuEG1CDnK0pUPbRrk5QhqDgkQNxP4Qqhz9xZe4";
    const std::string host = "www.affilinet-verzeichnis.de";
    const std::string userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.21 (KHTML, like Gecko) Chrome/25.0.1349.2 Safari/537.21";
    const std::string publicKey = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBANnylWw2vLY4hUn9w06zQKbhKBfvjFUCsdFlb6TdQhxb9RXWXuI4t31c+o8fYOv/s8q1LGP"
                                  "ga3DE1L/tHU4LENMCAwEAAQ==";
    const std::string signature = "nLH8Vbc1rzmy0Q+Xg+bvm43IEO42h8rq5D9C0WCn/Y3ykgAoV4npzm7eMlqBSwZBLA/0DuuVsfTJT9MOVaurcA==";
  };
}

TEST_F(FilterEngineTest, FilterCreation)
{
  auto& filterEngine = GetFilterEngine();
  AdblockPlus::Filter filter1 = filterEngine.GetFilter("foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, filter1.GetType());
  AdblockPlus::Filter filter2 = filterEngine.GetFilter("@@foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter2.GetType());
  AdblockPlus::Filter filter3 = filterEngine.GetFilter("example.com##foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE, filter3.GetType());
  AdblockPlus::Filter filter4 = filterEngine.GetFilter("example.com#@#foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION, filter4.GetType());
  AdblockPlus::Filter filter5 = filterEngine.GetFilter("  foo  ");
  ASSERT_EQ(filter1, filter5);
  AdblockPlus::Filter filter6 = filterEngine.GetFilter("example.com#?#foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE_EMULATION, filter6.GetType());
}

TEST_F(FilterEngineTest, AddRemoveFilters)
{
  auto& filterEngine = GetFilterEngine();
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  AdblockPlus::Filter filter = filterEngine.GetFilter("foo");
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
  filter.AddToList();
  ASSERT_EQ(1u, filterEngine.GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine.GetListedFilters()[0]);
  ASSERT_TRUE(filter.IsListed());
  filter.AddToList();
  ASSERT_EQ(1u, filterEngine.GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine.GetListedFilters()[0]);
  ASSERT_TRUE(filter.IsListed());
  filter.RemoveFromList();
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
  filter.RemoveFromList();
  ASSERT_EQ(0u, filterEngine.GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
}

TEST_F(FilterEngineTest, AddedSubscriptionIsEnabled)
{
  AdblockPlus::Subscription subscription = GetFilterEngine().GetSubscription("https://foo/");
  EXPECT_FALSE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, DisablingSubscriptionDisablesItAndFiresEvent)
{
  AdblockPlus::Subscription subscription = GetFilterEngine().GetSubscription("https://foo/");
  int eventFiredCounter = 0;
  GetFilterEngine().SetFilterChangeCallback([&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject)
  {
    if (eventName != "subscription.disabled" || subscriptionObject.GetProperty("url").AsString() != "https://foo/")
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
  AdblockPlus::Subscription subscription = GetFilterEngine().GetSubscription("https://foo/");
  EXPECT_FALSE(subscription.IsDisabled());
  subscription.SetDisabled(true);
  EXPECT_TRUE(subscription.IsDisabled());

  int eventFiredCounter = 0;
  GetFilterEngine().SetFilterChangeCallback([&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject)
  {
    if (eventName != "subscription.disabled" || subscriptionObject.GetProperty("url").AsString() != "https://foo/")
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
  ASSERT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  AdblockPlus::Subscription subscription = filterEngine.GetSubscription("https://foo/");
  ASSERT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
  subscription.AddToList();
  ASSERT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine.GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription.IsListed());
  subscription.AddToList();
  ASSERT_EQ(1u, filterEngine.GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine.GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription.IsListed());
  subscription.RemoveFromList();
  ASSERT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
  subscription.RemoveFromList();
  ASSERT_EQ(0u, filterEngine.GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
}

TEST_F(FilterEngineTest, SubscriptionUpdates)
{
  AdblockPlus::Subscription subscription = GetFilterEngine().GetSubscription("https://foo/");
  ASSERT_FALSE(subscription.IsUpdating());
  subscription.UpdateFilters();
}

TEST_F(FilterEngineTest, RecommendedSubscriptions)
{
  auto subscriptions = GetFilterEngine().FetchAvailableSubscriptions();
  EXPECT_FALSE(subscriptions.empty());

  for (const auto& cur : subscriptions)
  {
    EXPECT_FALSE(cur.IsListed());
    EXPECT_FALSE(cur.IsAA());
  }
}

TEST_F(FilterEngineTest, RecommendedSubscriptionsLanguages)
{
  auto subscriptions = GetFilterEngine().FetchAvailableSubscriptions();
  auto it = std::find_if(subscriptions.begin(), subscriptions.end(),
                        [] (const auto& cur) { return cur.GetTitle() == "RuAdList+EasyList"; });
  ASSERT_TRUE(it != subscriptions.end());
  auto languages = it->GetLanguages();
  ASSERT_EQ(2u, languages.size());
  EXPECT_EQ("ru", languages[0]);
  EXPECT_EQ("uk", languages[1]);
}

TEST_F(FilterEngineTest, RecommendedSubscriptionsLanguagesEmpty)
{
  auto subscriptions = GetFilterEngine().FetchAvailableSubscriptions();
  auto it = std::find_if(subscriptions.begin(), subscriptions.end(),
                        [] (const auto& cur) { return cur.GetTitle() == "EasyPrivacy"; });
  ASSERT_TRUE(it != subscriptions.end());
  auto languages = it->GetLanguages();
  EXPECT_EQ(0u, languages.size());
}

TEST_F(FilterEngineTest, Matches)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@notbanner.gif").AddToList();
  filterEngine.GetFilter("tpbanner.gif$third-party").AddToList();
  filterEngine.GetFilter("fpbanner.gif$~third-party").AddToList();
  filterEngine.GetFilter("combanner.gif$domain=example.com").AddToList();
  filterEngine.GetFilter("orgbanner.gif$domain=~example.com").AddToList();

  AdblockPlus::FilterPtr match1 = filterEngine.Matches("http://example.org/foobar.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_FALSE(match1);

  AdblockPlus::FilterPtr match2 = filterEngine.Matches("http://example.org/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match2->GetType());

  AdblockPlus::FilterPtr match3 = filterEngine.Matches("http://example.org/notbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match3->GetType());

  AdblockPlus::FilterPtr match4 = filterEngine.Matches("http://example.org/notbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match4);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match4->GetType());

  AdblockPlus::FilterPtr match5 = filterEngine.Matches("http://example.org/tpbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_FALSE(match5);

  AdblockPlus::FilterPtr match6 = filterEngine.Matches("http://example.org/fpbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_TRUE(match6);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match6->GetType());

  AdblockPlus::FilterPtr match7 = filterEngine.Matches("http://example.org/tpbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_TRUE(match7);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match7->GetType());

  AdblockPlus::FilterPtr match8 = filterEngine.Matches("http://example.org/fpbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_FALSE(match8);

  AdblockPlus::FilterPtr match9 = filterEngine.Matches("http://example.org/combanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_TRUE(match9);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match9->GetType());

  AdblockPlus::FilterPtr match10 = filterEngine.Matches("http://example.org/combanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_FALSE(match10);

  AdblockPlus::FilterPtr match11 = filterEngine.Matches("http://example.org/orgbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_FALSE(match11);

  AdblockPlus::FilterPtr match12 = filterEngine.Matches("http://example.org/orgbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_TRUE(match12);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match12->GetType());
}

TEST_F(FilterEngineTest, GenericblockHierarchy)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("@@||example.com^$genericblock,domain=example.com").AddToList();

  std::string currentUrl = "http://example.com/add.png";
  std::string parentUrl = "http://example.com/frame.html";
  std::vector<std::string> documentUrlsForCurrentUrl, documentUrlsForParentUrl;
  documentUrlsForCurrentUrl.push_back("http://example.com/frame.html");
  documentUrlsForCurrentUrl.push_back("http://example.net/index.html");
  documentUrlsForParentUrl.push_back("http://example.net/index.html");

  EXPECT_TRUE(filterEngine.IsGenericblockWhitelisted(currentUrl, documentUrlsForCurrentUrl));
  EXPECT_FALSE(filterEngine.IsGenericblockWhitelisted(parentUrl, documentUrlsForParentUrl));

  filterEngine.GetFilter("@@||example.com^$genericblock,domain=example.com").RemoveFromList();
  filterEngine.GetFilter("@@||example.net^$genericblock").AddToList();
  EXPECT_TRUE(filterEngine.IsGenericblockWhitelisted(parentUrl, documentUrlsForParentUrl));
}

/*
 * This test shows how genericblock filter option works:
 * Page http://testpages.adblockplus.org/en/exceptions/genericblock issues following requests:
 * 1) http://testpages.adblockplus.org/testcasefiles/genericblock/target-generic.jpg
 * 2) http://testpages.adblockplus.org/testcasefiles/genericblock/target-notgeneric.jpg
 *
 * Before genericblock filter is added ("@@||testpages.adblockplus.org/en/exceptions/genericblock$genericblock")
 * both requests are blocked.
 *
 * After genericblock filter is added only 2) is blocked as there is a site-specific filter for this request
 * ("/testcasefiles/genericblock/target-notgeneric.jpg$domain=testpages.adblockplus.org") and 1) passes as
 * it has only a generic filter ("/testcasefiles/genericblock/target-generic.jpg").
 */
TEST_F(FilterEngineTest, GenericblockMatch)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("/testcasefiles/genericblock/target-generic.jpg").AddToList();
  filterEngine.GetFilter("/testcasefiles/genericblock/target-notgeneric.jpg$domain=testpages.adblockplus.org").AddToList();
  const std::string urlGeneric = "http://testpages.adblockplus.org/testcasefiles/genericblock/target-generic.jpg";
  const std::string urlNotGeneric = "http://testpages.adblockplus.org/testcasefiles/genericblock/target-notgeneric.jpg";
  const std::string firstParent = "http://testpages.adblockplus.org/en/exceptions/genericblock/frame.html";
  AdblockPlus::IFilterEngine::ContentType contentType = AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE;
  std::vector<std::string> documentUrls, documentUrlsForGenericBlock;
  documentUrls.push_back("http://testpages.adblockplus.org/testcasefiles/genericblock/frame.html");
  documentUrls.push_back("http://testpages.adblockplus.org/en/exceptions/genericblock/");
  documentUrlsForGenericBlock.push_back("http://testpages.adblockplus.org/en/exceptions/genericblock/");

  bool specificOnly = filterEngine.IsGenericblockWhitelisted(firstParent, documentUrlsForGenericBlock);
  EXPECT_FALSE(specificOnly);

  AdblockPlus::FilterPtr match1 = filterEngine.Matches(urlNotGeneric, contentType, documentUrls, "", specificOnly);
  ASSERT_TRUE(match1);
  EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  specificOnly = filterEngine.IsGenericblockWhitelisted(firstParent, documentUrlsForGenericBlock);
  EXPECT_FALSE(specificOnly);

  AdblockPlus::FilterPtr match2 = filterEngine.Matches(urlGeneric, contentType, documentUrls, "", specificOnly);
  ASSERT_TRUE(match2);
  EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match2->GetType());

  // Now add genericblock filter and do the checks
  filterEngine.GetFilter("@@||testpages.adblockplus.org/en/exceptions/genericblock$genericblock").AddToList();

  specificOnly = filterEngine.IsGenericblockWhitelisted(firstParent, documentUrlsForGenericBlock);
  EXPECT_TRUE(specificOnly);

  match1 = filterEngine.Matches(urlNotGeneric, contentType, documentUrls, "", specificOnly);
  ASSERT_TRUE(match1); // This is still blocked as a site-specific blocking filter applies
  EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  specificOnly = filterEngine.IsGenericblockWhitelisted(firstParent, documentUrlsForGenericBlock);
  EXPECT_TRUE(specificOnly);

  match2 = filterEngine.Matches(urlGeneric, contentType, documentUrls, "", specificOnly);
  EXPECT_FALSE(match2); // Now with genericblock this request is not blocked
}

TEST_F(FilterEngineTest, GenericblockWithDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("@@||foo.example.com^$genericblock,domain=example.net").AddToList();
  filterEngine.GetFilter("@@||bar.example.com^$genericblock,domain=~example.net").AddToList();

  std::vector<std::string> documentUrls;
  documentUrls.push_back("http://example.net");

  EXPECT_TRUE(filterEngine.IsGenericblockWhitelisted("http://foo.example.com", documentUrls));
  EXPECT_FALSE(filterEngine.IsGenericblockWhitelisted("http://bar.example.com", documentUrls));
}

TEST_F(FilterEngineTest, Generichide)
{
  const char* url = "http://foo.com/bar";
  const char* docUrl = "http://foo.com/";

  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("@@bar.com$generichide").AddToList();
  AdblockPlus::FilterPtr match1 = filterEngine.Matches(url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE, docUrl);
  EXPECT_FALSE(match1);

  filterEngine.GetFilter("@@foo.com$generichide").AddToList();
  AdblockPlus::FilterPtr match2 = filterEngine.Matches(url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE, docUrl);
  ASSERT_TRUE(match2); // should be Filter instance
  EXPECT_EQ("@@foo.com$generichide", match2->GetRaw());
}

TEST_F(FilterEngineTest, MatchesOnWhitelistedDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@||example.org^$document").AddToList();

  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          "http://example.com/");
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          "http://example.org/");
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());
}

TEST_F(FilterEngineTest, MatchesWithContentTypeMask)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif.js$script,image").AddToList();
  filterEngine.GetFilter("@@notbanner.gif").AddToList();
  filterEngine.GetFilter("blockme").AddToList();
  filterEngine.GetFilter("@@||example.doc^$document").AddToList();
  filterEngine.GetFilter("||popexample.com^$popup").AddToList();

  EXPECT_TRUE(filterEngine.Matches("http://popexample.com/",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_POPUP, ""));

  EXPECT_FALSE(filterEngine.Matches("http://example.org/foobar.gif",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE, ""))
    << "another url should not match";

  EXPECT_FALSE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    /*mask*/ 0, "")) << "zero mask should not match (filter with some options)";

  EXPECT_FALSE(filterEngine.Matches("http://example.xxx/blockme",
    /*mask*/ 0, "")) << "zero mask should not match (filter without any option)";

  EXPECT_FALSE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one arbitrary flag in mask should not match";

  EXPECT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one of flags in mask should match";

  EXPECT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT, ""))
    << "both flags in mask should match";

  EXPECT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT |
    AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "both flags with another flag in mask should match";

  EXPECT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT |
    AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one of flags in mask should match";

  {
    AdblockPlus::FilterPtr filter;
    ASSERT_TRUE(filter = filterEngine.Matches("http://child.any/blockme",
      AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT |
      AdblockPlus::IFilterEngine::CONTENT_TYPE_OBJECT, "http://example.doc"))
      << "non-zero mask should match on whitelisted document";

    EXPECT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter->GetType());
  }

  {
    AdblockPlus::FilterPtr filter;
    ASSERT_TRUE(filter = filterEngine.Matches("http://example.doc/blockme",
      /*mask*/0, "http://example.doc"))
      << "zero mask should match when document is whitelisted";

    EXPECT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter->GetType());
  }
}

TEST_F(FilterEngineTest, MatchesNestedFrameRequest)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@adbanner.gif$domain=example.org").AddToList();

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://ads.com/frame/");
  documentUrls1.push_back("http://example.com/");
  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls1);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  std::vector<std::string> documentUrls2;
  documentUrls2.push_back("http://ads.com/frame/");
  documentUrls2.push_back("http://example.org/");
  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls2);
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());

  std::vector<std::string> documentUrls3;
  documentUrls3.push_back("http://example.org/");
  documentUrls3.push_back("http://ads.com/frame/");
  AdblockPlus::FilterPtr match3 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls3);
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match3->GetType());
}

TEST_F(FilterEngineTest, MatchesNestedFrameOnWhitelistedDomain)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@||example.org^$document,domain=ads.com").AddToList();

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://ads.com/frame/");
  documentUrls1.push_back("http://example.com/");
  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls1);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  std::vector<std::string> documentUrls2;
  documentUrls2.push_back("http://ads.com/frame/");
  documentUrls2.push_back("http://example.org/");
  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls2);
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());

  std::vector<std::string> documentUrls3;
  documentUrls3.push_back("http://example.org/");
  AdblockPlus::FilterPtr match3 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls3);
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match3->GetType());

  std::vector<std::string> documentUrls4;
  documentUrls4.push_back("http://example.org/");
  documentUrls4.push_back("http://ads.com/frame/");
  AdblockPlus::FilterPtr match4 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls4);
  ASSERT_TRUE(match4);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match4->GetType());

  std::vector<std::string> documentUrls5;
  documentUrls5.push_back("http://ads.com/frame/");
  documentUrls5.push_back("http://example.org/");
  documentUrls5.push_back("http://example.com/");
  AdblockPlus::FilterPtr match5 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls5);
  ASSERT_TRUE(match5);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match5->GetType());
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
  filterEngine.GetFilter("/script.js$sitekey=" + siteKey).AddToList();

  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://example.org/script.js", AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                         "http://example.org/");
  ASSERT_FALSE(match1) << "should not match without siteKey";

  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://example.org/script.img", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                         "http://example.org/", siteKey);
  ASSERT_FALSE(match2) << "should not match different content type";

  AdblockPlus::FilterPtr match3 =
    filterEngine.Matches("http://example.org/script.js", AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                         "http://example.org/", siteKey);
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match3->GetType());
}

TEST_F(FilterEngineTestSiteKey, MatchesWhitelistedSiteKey)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@||ads.com$image,sitekey=" + siteKey).AddToList();

  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                         "http://example.org/", siteKey);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match1->GetType());

  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://ads.com/adbanner.js", AdblockPlus::IFilterEngine::CONTENT_TYPE_SCRIPT,
                         "http://example.org/", siteKey);
  ASSERT_FALSE(match2) << "should not match different content type";
}

TEST_F(FilterEngineTestSiteKey, MatchesWhitelistedSiteKeyFromNestedFrameRequest)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adbanner.gif").AddToList();
  filterEngine.GetFilter("@@adbanner.gif$domain=example.org,sitekey=" + siteKey).AddToList();

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://ads.com/frame/");
  documentUrls1.push_back("http://example.com/");
  AdblockPlus::FilterPtr match1 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls1, siteKey);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  std::vector<std::string> documentUrls2;
  documentUrls2.push_back("http://ads.com/frame/");
  documentUrls2.push_back("http://example.org/");
  AdblockPlus::FilterPtr match2 =
    filterEngine.Matches("http://ads.com/adbanner.gif", AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls2, siteKey);
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());
}

TEST_F(FilterEngineTestSiteKey, IsDocAndIsElemhideWhitelsitedMatchesWhitelistedSiteKey)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("adframe").AddToList();
  auto docSiteKey = siteKey + "_document";
  auto elemhideSiteKey = siteKey + "_elemhide";
  filterEngine.GetFilter("@@$document,sitekey=" + docSiteKey).AddToList();
  filterEngine.GetFilter("@@$elemhide,sitekey=" + elemhideSiteKey).AddToList();

  {
    // normally the frame is not whitelisted
    std::vector<std::string> documentUrls;
    documentUrls.push_back("http://example.com/");
    documentUrls.push_back("http://ads.com/");
    { // no sitekey
      AdblockPlus::FilterPtr matchResult =
        filterEngine.Matches("http://my-ads.com/adframe", AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                              documentUrls);
      ASSERT_TRUE(matchResult);
      EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, matchResult->GetType());
    }
    { // random sitekey
      AdblockPlus::FilterPtr matchResult =
        filterEngine.Matches("http://my-ads.com/adframe", AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                              documentUrls, siteKey);
      ASSERT_TRUE(matchResult);
      EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, matchResult->GetType());
    }
    if (false) // TODO: should be enabled during DP-235
    { // the sitekey, but filter does not whitelist subdocument
      AdblockPlus::FilterPtr matchResult =
        filterEngine.Matches("http://my-ads.com/adframe", AdblockPlus::IFilterEngine::CONTENT_TYPE_SUBDOCUMENT,
                              documentUrls, docSiteKey);
      ASSERT_TRUE(matchResult);
      EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, matchResult->GetType());
    }
  }

  { // the frame itself
    std::vector<std::string> documentUrls;
    documentUrls.push_back("http://example.com/");
    documentUrls.push_back("http://ads.com/");
    // no sitekey
    EXPECT_FALSE(filterEngine.IsDocumentWhitelisted("http://my-ads.com/adframe", documentUrls));
    EXPECT_FALSE(filterEngine.IsElemhideWhitelisted("http://my-ads.com/adframe", documentUrls));
    // random sitekey and the correct sitekey
    EXPECT_FALSE(filterEngine.IsDocumentWhitelisted("http://my-ads.com/adframe", documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsDocumentWhitelisted("http://my-ads.com/adframe", documentUrls, docSiteKey));
    EXPECT_FALSE(filterEngine.IsElemhideWhitelisted("http://my-ads.com/adframe", documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsElemhideWhitelisted("http://my-ads.com/adframe", documentUrls, elemhideSiteKey));
  }

  { // the frame withing a whitelisted frame
    std::vector<std::string> documentUrls;
    documentUrls.push_back("http://example.com/");
    documentUrls.push_back("http:/my-ads.com/adframe");
    documentUrls.push_back("http://ads.com/");
    // no sitekey
    EXPECT_FALSE(filterEngine.IsDocumentWhitelisted("http://some-ads.com", documentUrls));
    EXPECT_FALSE(filterEngine.IsElemhideWhitelisted("http://some-ads.com", documentUrls));
    // random sitekey and the correct sitekey
    EXPECT_FALSE(filterEngine.IsDocumentWhitelisted("http://some-ads.com", documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsDocumentWhitelisted("http://some-ads.com", documentUrls, docSiteKey));
    EXPECT_FALSE(filterEngine.IsElemhideWhitelisted("http://some-ads.com", documentUrls, siteKey));
    EXPECT_TRUE(filterEngine.IsElemhideWhitelisted("http://some-ads.com", documentUrls, elemhideSiteKey));
  }
}

TEST_F(FilterEngineTest, FirstRunFlag)
{
  ASSERT_FALSE(GetFilterEngine().IsFirstRun());
}

TEST_F(FilterEngineTestNoData, FirstRunFlag)
{
  ASSERT_TRUE(GetFilterEngine().IsFirstRun());
}

TEST_F(FilterEngineTest, SetRemoveFilterChangeCallback)
{
  auto& filterEngine = GetFilterEngine();
  int timesCalled = 0;
  filterEngine.SetFilterChangeCallback([&timesCalled](const std::string&, AdblockPlus::JsValue&&)
  {
    timesCalled++;
  });
  filterEngine.GetFilter("foo").AddToList();
  EXPECT_EQ(1, timesCalled);

  // we want to actually check the call count didn't change.
  filterEngine.RemoveFilterChangeCallback();
  filterEngine.GetFilter("foo").RemoveFromList();
  EXPECT_EQ(1, timesCalled);
}

TEST_F(FilterEngineTest, DocumentWhitelisting)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("@@||example.org^$document").AddToList();
  filterEngine.GetFilter("@@||example.com^$document,domain=example.de").AddToList();

  ASSERT_TRUE(filterEngine.IsDocumentWhitelisted(
      "http://example.org",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine.IsDocumentWhitelisted(
      "http://example.co.uk",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine.IsDocumentWhitelisted(
      "http://example.com",
      std::vector<std::string>()));

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://example.de");

  ASSERT_TRUE(filterEngine.IsDocumentWhitelisted(
      "http://example.com",
      documentUrls1));

  ASSERT_FALSE(filterEngine.IsDocumentWhitelisted(
      "http://example.co.uk",
      documentUrls1));

  filterEngine.GetFilter("||testpages.adblockplus.org/testcasefiles/document/*").AddToList();
  filterEngine.GetFilter("@@testpages.adblockplus.org/en/exceptions/document^$document").AddToList();

  // Frames hierarchy:
  // - http://testpages.adblockplus.org/en/exceptions/document
  //  - http://testpages.adblockplus.org/testcasefiles/document/frame.html
  //   - http://testpages.adblockplus.org/testcasefiles/document/image.jpg

  documentUrls1.clear();
  documentUrls1.push_back("http://testpages.adblockplus.org/en/exceptions/document");

  // Check for http://testpages.adblockplus.org/testcasefiles/document/image.jpg
  EXPECT_TRUE(filterEngine.IsDocumentWhitelisted(
      "http://testpages.adblockplus.org/testcasefiles/document/frame.html",
      documentUrls1));
}

TEST_F(FilterEngineTest, ElemhideWhitelisting)
{
  auto& filterEngine = GetFilterEngine();
  filterEngine.GetFilter("@@||example.org^$elemhide").AddToList();
  filterEngine.GetFilter("@@||example.com^$elemhide,domain=example.de").AddToList();

  ASSERT_TRUE(filterEngine.IsElemhideWhitelisted(
      "http://example.org",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine.IsElemhideWhitelisted(
      "http://example.co.uk",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine.IsElemhideWhitelisted(
      "http://example.com",
      std::vector<std::string>()));

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://example.de");

  ASSERT_TRUE(filterEngine.IsElemhideWhitelisted(
      "http://example.com",
      documentUrls1));

  ASSERT_FALSE(filterEngine.IsElemhideWhitelisted(
      "http://example.co.uk",
      documentUrls1));

  filterEngine.GetFilter("testpages.adblockplus.org##.testcase-ex-elemhide").AddToList();
  filterEngine.GetFilter("@@testpages.adblockplus.org/en/exceptions/elemhide^$elemhide").AddToList();
  filterEngine.GetFilter("||testpages.adblockplus.org/testcasefiles/elemhide/image.jpg").AddToList();

  // Frames hierarchy:
  // - http://testpages.adblockplus.org/en/exceptions/elemhide
  //  - http://testpages.adblockplus.org/testcasefiles/elemhide/frame.html
  //   - http://testpages.adblockplus.org/testcasefiles/elemhide/image.jpg

  documentUrls1.clear();
  documentUrls1.push_back("http://testpages.adblockplus.org/en/exceptions/elemhide");

  EXPECT_TRUE(filterEngine.IsElemhideWhitelisted(
      "http://testpages.adblockplus.org/testcasefiles/elemhide/frame.html",
      documentUrls1));
  auto filter = filterEngine.Matches(
      "http://testpages.adblockplus.org/testcasefiles/elemhide/image.jpg",
      AdblockPlus::IFilterEngine::CONTENT_TYPE_IMAGE,
      documentUrls1);
  ASSERT_NE(nullptr, filter);
  EXPECT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, filter->GetType());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetEmpty)
{
  auto& filterEngine = GetFilterEngine();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");

  EXPECT_TRUE(sheet.empty());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheet)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<std::string> filters =
  {
    // other type of filters
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
    "othersite.com###testcase-eh-id"
  };

  for (const auto& filter : filters)
    filterEngine.GetFilter(filter).AddToList();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");

  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n#testcase-eh-id, .testcase-eh-class, .testcase-container > .testcase-eh-descendant, foo, testneg {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetSingleGeneric)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors
  filterEngine.GetFilter("###testcase-eh-id").AddToList();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("");

  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetSingleDomain)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors
  filterEngine.GetFilter("example.org##.testcase - eh - class").AddToList();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");

  EXPECT_EQ(".testcase - eh - class {display: none !important;}\n", sheet);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetDup)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding selectors - duplicates
  filterEngine.GetFilter("example.org###dup").AddToList();
  filterEngine.GetFilter("example.org###dup").AddToList();
  filterEngine.GetFilter("othersite.org###dup").AddToList();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");

  // no dups
  EXPECT_EQ("#dup {display: none !important;}\n", sheet);

  // this makes duplicates
  filterEngine.GetFilter("~foo.example.org,example.org###dup").AddToList();
  filterEngine.GetFilter("~bar.example.org,example.org###dup").AddToList();

  std::string sheetDup = filterEngine.GetElementHidingStyleSheet("example.org");

  // dups
  EXPECT_EQ("#dup, #dup, #dup {display: none !important;}\n", sheetDup);

  std::string sheetBar = filterEngine.GetElementHidingStyleSheet("bar.example.org");
  EXPECT_EQ("#dup, #dup {display: none !important;}\n", sheetBar);
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetDiff)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("example1.org###testcase-eh-id").AddToList();
  filterEngine.GetFilter("example2.org###testcase-eh-id").AddToList();

  std::string sheet1 = filterEngine.GetElementHidingStyleSheet("example1.org");
  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet1);

  std::string sheet2 = filterEngine.GetElementHidingStyleSheet("example2.org");
  EXPECT_EQ("#testcase-eh-id {display: none !important;}\n", sheet2);

  std::string sheetGen = filterEngine.GetElementHidingStyleSheet("");
  EXPECT_TRUE(sheetGen.empty());

  std::string sheetNonExisting = filterEngine.GetElementHidingStyleSheet("non-existing-domain.com");
  EXPECT_TRUE(sheetNonExisting.empty());
}

TEST_F(FilterEngineTest, ElementHidingStyleSheetGenerichide)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("##.testcase-generichide-generic").AddToList();
  filterEngine.GetFilter("example.org##.testcase-generichide-notgeneric").AddToList();
  filterEngine.GetFilter("@@||example.org$generichide").AddToList();

  std::string sheet = filterEngine.GetElementHidingStyleSheet("example.org");

  EXPECT_EQ(".testcase-generichide-generic {display: none !important;}\n.testcase-generichide-notgeneric {display: none !important;}\n", sheet);

  std::string sheetSpecificOnly = filterEngine.GetElementHidingStyleSheet("example.org", true);

  EXPECT_EQ(".testcase-generichide-notgeneric {display: none !important;}\n", sheetSpecificOnly);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListEmpty)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<IFilterEngine::EmulationSelector> sels = filterEngine.GetElementHidingEmulationSelectors("example.org");
  EXPECT_TRUE(sels.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsWhitelist)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("example.org#?#foo").AddToList();

  // before whitelisting
  std::vector<IFilterEngine::EmulationSelector> selsBeforeWhitelisting = filterEngine.GetElementHidingEmulationSelectors("example.org");
  ASSERT_EQ(1u, selsBeforeWhitelisting.size());
  EXPECT_EQ("foo", selsBeforeWhitelisting[0].selector);
  EXPECT_EQ("example.org#?#foo", selsBeforeWhitelisting[0].text);

  // whitelist it
  filterEngine.GetFilter("example.org#@#foo").AddToList();

  std::vector<IFilterEngine::EmulationSelector> selsAfterWhitelisting = filterEngine.GetElementHidingEmulationSelectors("example.org");
  EXPECT_TRUE(selsAfterWhitelisting.empty());

  // add another filter
  filterEngine.GetFilter("example.org#?#another").AddToList();

  std::vector<IFilterEngine::EmulationSelector> selsAnother = filterEngine.GetElementHidingEmulationSelectors("example.org");
  ASSERT_EQ(1u, selsAnother.size());
  EXPECT_EQ("another", selsAnother[0].selector);
  EXPECT_EQ("example.org#?#another", selsAnother[0].text);

  // check another domain
  filterEngine.GetFilter("example2.org#?#foo").AddToList();

  std::vector<IFilterEngine::EmulationSelector> sels2 = filterEngine.GetElementHidingEmulationSelectors("example2.org");
  ASSERT_EQ(1u, sels2.size());
  EXPECT_EQ("foo", sels2[0].selector);
  EXPECT_EQ("example2.org#?#foo", sels2[0].text);

  // check the type of the whitelist (exception) filter
  auto filter = filterEngine.GetFilter("example.org#@#bar");
  EXPECT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION, filter.GetType());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsList)
{
  auto& filterEngine = GetFilterEngine();

  std::vector<std::string> filters =
  {
    // other type of filters
    "/testcasefiles/blocking/addresspart/abptestcasepath/",
    "example.org###testcase-eh-id",

    // element hiding emulation selectors
    "example.org#?#div:-abp-properties(width: 213px)",
    "example.org#?#div:-abp-has(>div>img.testcase-es-has)",
    "example.org#?#span:-abp-contains(ESContainsTarget)",
    "~foo.example.org,example.org#?#div:-abp-properties(width: 213px)",
    "~othersiteneg.org#?#div:-abp-properties(width: 213px)",

    // whitelisted
    "example.org#@#foo",

    // other site
    "othersite.com###testcase-eh-id"
  };

  for (const auto& filter : filters)
    filterEngine.GetFilter(filter).AddToList();

  std::vector<IFilterEngine::EmulationSelector> sels = filterEngine.GetElementHidingEmulationSelectors("example.org");

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

  std::vector<IFilterEngine::EmulationSelector> sels2 = filterEngine.GetElementHidingEmulationSelectors("foo.example.org");
  ASSERT_EQ(3u, sels2.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels2[0].selector);
  EXPECT_EQ("div:-abp-has(>div>img.testcase-es-has)", sels2[1].selector);
  EXPECT_EQ("span:-abp-contains(ESContainsTarget)", sels2[2].selector);

  EXPECT_EQ("example.org#?#div:-abp-properties(width: 213px)", sels2[0].text);
  EXPECT_EQ("example.org#?#div:-abp-has(>div>img.testcase-es-has)", sels2[1].text);
  EXPECT_EQ("example.org#?#span:-abp-contains(ESContainsTarget)", sels2[2].text);

  std::vector<IFilterEngine::EmulationSelector> sels3 = filterEngine.GetElementHidingEmulationSelectors("othersiteneg.org");
  ASSERT_EQ(0u, sels3.size());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListSingleDomain)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selector
  filterEngine.GetFilter("example.org#?#div:-abp-properties(width: 213px)").AddToList();

  std::vector<IFilterEngine::EmulationSelector> sels = filterEngine.GetElementHidingEmulationSelectors("example.org");

  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels[0].selector);
  EXPECT_EQ("example.org#?#div:-abp-properties(width: 213px)", sels[0].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListNoDuplicates)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selectors - duplicates
  filterEngine.GetFilter("example.org#?#dup").AddToList();
  filterEngine.GetFilter("example.org#?#dup").AddToList();
  filterEngine.GetFilter("othersite.org#?#dup").AddToList();
  filterEngine.GetFilter("~foo.example.org#?#dup").AddToList();

  std::vector<IFilterEngine::EmulationSelector> sels = filterEngine.GetElementHidingEmulationSelectors("example.org");

  // no dups
  ASSERT_EQ(1u, sels.size());
  EXPECT_EQ("dup", sels[0].selector);
  EXPECT_EQ("example.org#?#dup", sels[0].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListDuplicates)
{
  auto& filterEngine = GetFilterEngine();

  // element hiding emulation selectors - duplicates
  filterEngine.GetFilter("example.org#?#dup").AddToList();
  filterEngine.GetFilter("~foo.example.org,example.org#?#dup").AddToList();
  filterEngine.GetFilter("~bar.example.org,example.org#?#dup").AddToList();

  std::vector<IFilterEngine::EmulationSelector> selsDups = filterEngine.GetElementHidingEmulationSelectors("example.org");

  // dups
  ASSERT_EQ(3u, selsDups.size());
  EXPECT_EQ("dup", selsDups[0].selector);
  EXPECT_EQ("dup", selsDups[1].selector);
  EXPECT_EQ("dup", selsDups[2].selector);

  EXPECT_EQ("example.org#?#dup", selsDups[0].text);
  EXPECT_EQ("~foo.example.org,example.org#?#dup", selsDups[1].text);
  EXPECT_EQ("~bar.example.org,example.org#?#dup", selsDups[2].text);

  std::vector<IFilterEngine::EmulationSelector> selsBar = filterEngine.GetElementHidingEmulationSelectors("bar.example.org");
  ASSERT_EQ(2u, selsBar.size());
  EXPECT_EQ("dup", selsBar[0].selector);
  EXPECT_EQ("dup", selsBar[1].selector);

  EXPECT_EQ("example.org#?#dup", selsBar[0].text);
  EXPECT_EQ("~foo.example.org,example.org#?#dup", selsBar[1].text);
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsListDiff)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("example1.org#?#div:-abp-properties(width: 213px)").AddToList();
  filterEngine.GetFilter("example2.org#?#div:-abp-properties(width: 213px)").AddToList();
  // whitelisted
  filterEngine.GetFilter("example2.org#@#div:-abp-properties(width: 213px)").AddToList();

  std::vector<IFilterEngine::EmulationSelector> sels1 = filterEngine.GetElementHidingEmulationSelectors("example1.org");
  ASSERT_EQ(1u, sels1.size());
  EXPECT_EQ("div:-abp-properties(width: 213px)", sels1[0].selector);
  EXPECT_EQ("example1.org#?#div:-abp-properties(width: 213px)", sels1[0].text);

  std::vector<IFilterEngine::EmulationSelector> sels2 = filterEngine.GetElementHidingEmulationSelectors("example2.org");
  ASSERT_TRUE(sels2.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsGeneric)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("example1.org#?#foo").AddToList();
  filterEngine.GetFilter("example2.org#@#bar").AddToList();

  // there are no generic el-hiding emulation filters.
  // this should have no effect on selectors returned and the type should be invalid
  auto genFilter = filterEngine.GetFilter("#?#foo");
  genFilter.AddToList();

  EXPECT_EQ(AdblockPlus::Filter::TYPE_INVALID, genFilter.GetType());

  std::vector<IFilterEngine::EmulationSelector> selsGen = filterEngine.GetElementHidingEmulationSelectors("");
  EXPECT_TRUE(selsGen.empty());
}

TEST_F(FilterEngineTest, ElementHidingEmulationSelectorsNonExisting)
{
  auto& filterEngine = GetFilterEngine();

  filterEngine.GetFilter("example1.org#?#foo").AddToList();
  filterEngine.GetFilter("example2.org#@#bar").AddToList();

  std::vector<IFilterEngine::EmulationSelector> selsNonExisting = filterEngine.GetElementHidingEmulationSelectors("non-existing-domain.com");
  EXPECT_TRUE(selsNonExisting.empty());
}

class TestElement : public IElement
{
public:
  explicit TestElement(const std::map<std::string, std::string>& attributes,
                       const std::vector<TestElement>& children = {})
    : data(attributes),
      subelemets(children)
  {}

  const std::string& GetLocalName() const override
    { return GetAttribute("_name"); }

  const std::string& GetAttribute(const std::string& name) const override
  {
    auto it = data.find(name);
    static std::string empty;
    return it == data.end() ? empty : it->second;
  }

  const std::string& GetDocumentLocation() const override
    { return GetAttribute("_url"); }

  const std::vector<const IElement*> GetChildren() const override
  {
    std::vector<const IElement*> res;
    std::transform(subelemets.begin(), subelemets.end(),
                   std::back_inserter(res), [] (const auto& cur) { return &cur; });
    return res;
  }

private:
  std::map<std::string, std::string> data;
  std::vector<TestElement> subelemets;
};

TEST_F(FilterEngineTest, ComposeFilterSuggestionsClass)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page"},
                        {"_name", "img"},
                        {"class", "-img   _glyph"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("test.com##.\\-img._glyph", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsAttribute)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page"},
                        {"_name", "img"},
                        {"id", "gb_va"},
                        {"src", "data:abcd"},
                        {"style", "width:109px;height:40px"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(2u, res.size());
  EXPECT_EQ("test.com###gb_va", res[0]);
  EXPECT_EQ("test.com##img[src=\"data:abcd\"]", res[1]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsUrls)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page"},
                        {"_name", "img"},
                        {"id", "gb_va"},
                        {"src", "https://www.static.test.com/icon1.png"},
                        {"srcset", "https://www.static.test.com/icon1.png x1, http://test.com/ui/icon2.png x2, data:abcd"},
                        {"style", "width:109px;height:40px"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);

  ASSERT_EQ(2u, res.size());
  EXPECT_EQ("||static.test.com/icon1.png", res[0]);
  EXPECT_EQ("||test.com/ui/icon2.png", res[1]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsStyle)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page"},
                        {"_name", "div"},
                        {"style", "width:109px;height:40px"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("test.com##div[style=\"width:109px;height:40px\"]", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsBaseUrl)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page/"},
                        {"_name", "img"},
                        {"src", "/icon1.png"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/icon1.png", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsIgnoreWrongProtocol)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page/"},
                        {"_name", "img"},
                        {"srcset", "data:abcd"}
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  EXPECT_EQ(0u, res.size());
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForObjectElement)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page/"},
                        {"_name", "object"}
                      },
                      {
                        TestElement({
                          {"_name", "param"},
                          {"name", "src"},
                          {"value", "/data1"}
                        }),
                        TestElement({
                          {"_name", "param"},
                          {"name", "src1"},
                          {"value", "/data2"}
                        }),
                        TestElement({
                          {"_name", "div"},
                          {"name", "src"},
                          {"value", "/data3"}
                        })
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/data1", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForObjectElementData)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page/"},
                        {"_name", "object"},
                        {"data", "data4"}
                      },
                      {
                        TestElement({
                          {"_name", "param"},
                          {"name", "src"},
                          {"value", "/data1"}
                        }),
                        TestElement({
                          {"_name", "param"},
                          {"name", "src1"},
                          {"value", "/data2"}
                        }),
                        TestElement({
                          {"_name", "div"},
                          {"name", "src"},
                          {"value", "/data3"}
                        })
                      });

  auto res = filterEngine.ComposeFilterSuggestions(&element);
  ASSERT_EQ(1u, res.size());
  EXPECT_EQ("||test.com/page/data4", res[0]);
}

TEST_F(FilterEngineTest, ComposeFilterSuggestionsForMediaElement)
{
  auto& filterEngine = GetFilterEngine();
  TestElement element({
                        {"_url", "https://test.com/page/"},
                        {"_name", "video"},
                        {"poster", "/img1.png"}
                      },
                      {
                        TestElement({
                          {"_name", "source"},
                          {"src", "/data1"}
                        }),
                        TestElement({
                          {"_name", "track"},
                          {"src", "/data2"}
                        }),
                        TestElement({
                          {"_name", "else"},
                          {"src", "/data3"}
                        }),
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
  const std::string langSubscriptionUrl = "https://easylist-downloads.adblockplus.org/easylistchina+easylist.txt";
  InitPlatformAndAppInfo(appInfo);
  auto& filterEngine = CreateFilterEngine();
  const auto subscriptions = filterEngine.GetListedSubscriptions();
  ASSERT_EQ(2u, subscriptions.size());
  std::unique_ptr<Subscription> aaSubscription;
  std::unique_ptr<Subscription> langSubscription;
  if (subscriptions[0].IsAA())
  {
    aaSubscription.reset(new Subscription(subscriptions[0]));
    langSubscription.reset(new Subscription(subscriptions[1]));
  }
  else if (subscriptions[1].IsAA())
  {
    aaSubscription.reset(new Subscription(subscriptions[1]));
    langSubscription.reset(new Subscription(subscriptions[0]));
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
  createParams.preconfiguredPrefs.emplace("first_run_subscription_auto_select", GetJsEngine().NewValue(false));
  auto& filterEngine = CreateFilterEngine(createParams);
  const auto subscriptions = filterEngine.GetListedSubscriptions();
  EXPECT_EQ(0u, subscriptions.size());
  EXPECT_FALSE(filterEngine.IsAAEnabled());
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
    default:
      ;
    }
    return os;
  }

  enum class Action
  {
    disable, enable, remove
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
    default:
      ;
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
    os << "initial AA: " << params.initialAAStatus
       << " action: " << params.action
       << " expected AA: " << params.expectedAAStatus;
    return os;
  }
  class Test : public FilterEngineTest
             , public ::testing::WithParamInterface<::testing::tuple<Parameters, /*number of other subscriptions*/uint8_t>>
  {
  public:
    static std::vector<Parameters> VaryPossibleCases()
    {
      // AA API test matrix
      // each column but other-subs is about AA subscription
      // enabled exists other-subs action  => expected
      //                                   => everywhere no effect on other subs
      // 1.
      // false   false  false      disable => no effect
      // false   false  false      enable  => add and enable
      //
      // false   false  true       disable => no effect
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
      retValue.emplace_back(Parameters(AAStatus::absent, Action::disable));
      retValue.emplace_back(Parameters(AAStatus::absent, Action::enable).expect(AAStatus::enabled));
      // 2.
      retValue.emplace_back(Parameters(AAStatus::disabled_present, Action::disable));
      retValue.emplace_back(Parameters(AAStatus::disabled_present, Action::enable).expect(AAStatus::enabled));
      // 3.
      retValue.emplace_back(Parameters(AAStatus::enabled, Action::disable).expect(AAStatus::disabled_present));
      retValue.emplace_back(Parameters(AAStatus::enabled, Action::enable));
      // 4.
      retValue.emplace_back(Parameters(AAStatus::disabled_present, Action::remove).expect(AAStatus::absent));
      retValue.emplace_back(Parameters(AAStatus::enabled, Action::remove).expect(AAStatus::absent));
      // since AA should not affect other subscriptions, the number of other
      // subscriptions is not specified here, it goes as another item in test parameters tuple.
      return retValue;
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
        subscription.AddToList();
        const auto subscriptions = filterEngine.GetListedSubscriptions();
        ASSERT_EQ(1u, subscriptions.size());
        EXPECT_FALSE(subscriptions[0].IsAA());
        EXPECT_EQ(kOtherSubscriptionUrl, subscriptions[0].GetUrl());
      }
      if (isAASatusPresent(aaStatus))
      {
        filterEngine.SetAAEnabled(true); // add AA by enabling it
        if (aaStatus == AAStatus::disabled_present)
        {
          filterEngine.SetAAEnabled(false);
        }
        testSubscriptionState(aaStatus, otherSubscriptionsNumber);
      }
    }
    bool isAASatusPresent(AAStatus aaStatus)
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

      std::unique_ptr<Subscription> aaSubscription;
      std::unique_ptr<Subscription> otherSubscription;
      auto subscriptions = filterEngine.GetListedSubscriptions();
      for (auto& subscription : subscriptions)
      {
        auto& dstSubscription = subscription.IsAA() ? aaSubscription : otherSubscription;
        dstSubscription.reset(new Subscription(std::move(subscription)));
      }
      if (otherSubscriptionsNumber == 1u)
      {
        if (isAASatusPresent(aaStatus))
        {
          EXPECT_EQ(2u, subscriptions.size());
          EXPECT_TRUE(aaSubscription);
          EXPECT_TRUE(otherSubscription);
        }
        else
        {
          EXPECT_EQ(1u, subscriptions.size());
          EXPECT_FALSE(aaSubscription);
          EXPECT_TRUE(otherSubscription);
        }
      }
      else if (otherSubscriptionsNumber == 0u)
      {
        if (isAASatusPresent(aaStatus))
        {
          EXPECT_EQ(1u, subscriptions.size());
          EXPECT_TRUE(aaSubscription);
          EXPECT_FALSE(otherSubscription);
        }
        else
        {
          EXPECT_EQ(0u, subscriptions.size());
        }
      }
    }
  };

  INSTANTIATE_TEST_SUITE_P(AA_ApiTests, Test,
    ::testing::Combine(::testing::ValuesIn(Test::VaryPossibleCases()), ::testing::Values<uint8_t>(0, 1)));

  TEST_P(Test, VaryPossibleCases) {
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
      std::unique_ptr<Subscription> aaSubscription;
      for (auto& subscription : filterEngine.GetListedSubscriptions())
      {
        if (subscription.IsAA())
        {
          aaSubscription.reset(new Subscription(std::move(subscription)));
          break;
        }
      }
      ASSERT_TRUE(aaSubscription);
      aaSubscription->RemoveFromList();
    }

    testSubscriptionState(parameter.expectedAAStatus, otherSubscriptionsNumber);
  }
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, AbsentCallbackAllowsUpdating)
{
  createParams.isSubscriptionDownloadAllowedCallback = FilterEngineFactory::IsConnectionAllowedAsyncCallback();
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_EQ(1, subscription.GetFilterCount());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, AllowingCallbackAllowsUpdating)
{
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
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
  EXPECT_EQ(0, subscription.GetFilterCount());
  EXPECT_EQ(1u, capturedConnectionTypes.size());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, PredefinedAllowedConnectionTypeIsPassedToCallback)
{
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.insert(std::make_pair("allowed_connection_type",
    GetJsEngine().NewValue(predefinedAllowedConnectionType)));
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
  EXPECT_EQ(1, subscription.GetFilterCount());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, ConfiguredConnectionTypeIsPassedToCallback)
{
  // IFilterEngine::RemoveSubscription is not usable here because subscriptions
  // are cached internally by URL. So, different URLs are used in diffirent
  // checks.
  {
    std::string predefinedAllowedConnectionType = "non-metered";
    createParams.preconfiguredPrefs.insert(std::make_pair(
      "allowed_connection_type", GetJsEngine().NewValue(predefinedAllowedConnectionType)));
    auto subscription = EnsureExampleSubscriptionAndForceUpdate();
    EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
    EXPECT_EQ(1, subscription.GetFilterCount());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
  }
  capturedConnectionTypes.clear();
  {
    // set no value
    GetFilterEngine().SetAllowedConnectionType(nullptr);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subA");
    EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
    EXPECT_EQ(1, subscription.GetFilterCount());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_FALSE(capturedConnectionTypes[0].first);
    subscription.RemoveFromList();
  }
  capturedConnectionTypes.clear();
  {
    // set some value
    std::string testConnection = "test connection";
    GetFilterEngine().SetAllowedConnectionType(&testConnection);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subB");
    EXPECT_EQ("synchronize_ok", subscription.GetSynchronizationStatus());
    EXPECT_EQ(1, subscription.GetFilterCount());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(testConnection, capturedConnectionTypes[0].second);
  }
}
