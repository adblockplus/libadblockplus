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

#include "BaseJsTest.h"
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
  class VeryLazyFileSystem : public LazyFileSystem
  {
  public:
    StatResult Stat(const std::string& path) const
    {
      return StatResult();
    }
  };

  template<class FileSystem, class LogSystem>
  class FilterEngineTestGeneric : public ::testing::Test
  {
  protected:
    FilterEnginePtr filterEngine;

    void SetUp() override
    {
      JsEngineCreationParameters jsEngineParams;
      jsEngineParams.fileSystem.reset(new FileSystem());
      jsEngineParams.logSystem.reset(new LogSystem());
      jsEngineParams.timer.reset(new NoopTimer());
      jsEngineParams.webRequest.reset(new NoopWebRequest());
      auto jsEngine = CreateJsEngine(std::move(jsEngineParams));
      filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);
    }
    void TearDown() override
    {
      // Workaround for issue 5198
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  };

  typedef FilterEngineTestGeneric<LazyFileSystem, AdblockPlus::DefaultLogSystem> FilterEngineTest;
  typedef FilterEngineTestGeneric<VeryLazyFileSystem, LazyLogSystem> FilterEngineTestNoData;

  class UpdaterTest : public ::testing::Test
  {
  protected:
    class MockWebRequest : public AdblockPlus::WebRequest
    {
    public:
      AdblockPlus::ServerResponse response;

      AdblockPlus::ServerResponse GET(const std::string& url,
          const AdblockPlus::HeaderList& requestHeaders) const
      {
        return response;
      }
    };

    std::shared_ptr<MockWebRequest> mockWebRequest;
    FilterEnginePtr filterEngine;

    void SetUp()
    {
      JsEngineCreationParameters jsEngineParams;
      jsEngineParams.appInfo.name = "test";
      jsEngineParams.appInfo.version = "1.0.1";
      jsEngineParams.timer = CreateDefaultTimer();
      jsEngineParams.fileSystem.reset(new LazyFileSystem());
      AdblockPlus::JsEnginePtr jsEngine = CreateJsEngine(std::move(jsEngineParams));
      jsEngine->SetWebRequest(mockWebRequest = std::make_shared<MockWebRequest>());
      filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);
    }
  };

  class FilterEngineWithFreshFolder : public ::testing::Test
  {
  protected:
    FileSystemPtr fileSystem;
    std::weak_ptr<JsEngine> weakJsEngine;

    void SetUp() override
    {
      fileSystem.reset(new DefaultFileSystem());
      // Since there is neither in memory FS nor functionality to work with
      // directories use the hack: manually clean the directory.
      removeFileIfExists("patterns.ini");
      removeFileIfExists("prefs.json");
    }
    JsEnginePtr createJsEngine(const AppInfo& appInfo = AppInfo())
    {
      JsEngineCreationParameters jsEngineParams;
      jsEngineParams.appInfo = appInfo;
      jsEngineParams.fileSystem = fileSystem;
      jsEngineParams.logSystem.reset(new LazyLogSystem());
      jsEngineParams.timer.reset(new NoopTimer());
      jsEngineParams.webRequest.reset(new NoopWebRequest());
      auto jsEngine = CreateJsEngine(std::move(jsEngineParams));
      weakJsEngine = jsEngine;
      return jsEngine;
    }
    void TearDown() override
    {
      removeFileIfExists("patterns.ini");
      removeFileIfExists("prefs.json");
      fileSystem.reset();
    }
    void removeFileIfExists(const std::string& path)
    {
      // Hack: allow IO to finish currently running operations, in particular
      // writing into files. Otherwise we get "Permission denied".
      auto safeRemove = [this, &path]()->bool
      {
        try
        {
          if (fileSystem->Stat(path).exists)
            fileSystem->Remove(path);
          return true;
        }
        catch (...)
        {
          return false;
        }
      };
      int i = 5;
      while ((i-- > 0 && weakJsEngine.lock()) || !safeRemove())
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  };

  class FilterEngineIsSubscriptionDownloadAllowedTest : public ::testing::Test
  {
  protected:
    typedef std::vector<std::pair<bool, std::string>> ConnectionTypes;
    DelayedWebRequest::SharedTasks webRequestTasks;
    DelayedTimer::SharedTasks timerTasks;
    FilterEngine::CreationParameters createParams;
    ConnectionTypes capturedConnectionTypes;
    bool isConnectionAllowed;
    std::vector<std::function<void(bool)>> isSubscriptionDowloadAllowedCallbacks;
    FilterEnginePtr filterEngine;
    JsEnginePtr jsEngine;

    void SetUp()
    {
      isConnectionAllowed = true;

      JsEngineCreationParameters jsEngineParams;
      jsEngineParams.logSystem.reset(new LazyLogSystem());
      jsEngineParams.fileSystem.reset(new LazyFileSystem());
      jsEngineParams.timer = DelayedTimer::New(timerTasks);
      jsEngineParams.webRequest = DelayedWebRequest::New(webRequestTasks);
      jsEngine = CreateJsEngine(std::move(jsEngineParams));

      createParams.preconfiguredPrefs.emplace("first_run_subscription_auto_select", jsEngine->NewValue(false));

      createParams.isSubscriptionDowloadAllowedCallback = [this](const std::string* allowedConnectionType,
        const std::function<void(bool)>& isSubscriptionDowloadAllowedCallback){
        capturedConnectionTypes.emplace_back(!!allowedConnectionType, allowedConnectionType ? *allowedConnectionType : std::string());
        isSubscriptionDowloadAllowedCallbacks.emplace_back(isSubscriptionDowloadAllowedCallback);
      };
    }

    Subscription EnsureExampleSubscriptionAndForceUpdate(const std::string& apppendToUrl = std::string())
    {
      auto subscriptionUrl = "http://example" + apppendToUrl;
      bool isSubscriptionDownloadStatusReceived = false;
      if (!filterEngine)
      {
        filterEngine = FilterEngine::Create(jsEngine, createParams);
        filterEngine->SetFilterChangeCallback([&isSubscriptionDownloadStatusReceived, &subscriptionUrl](const std::string& action, JsValue&& item)
        {
          if (action == "subscription.downloadStatus" && item.GetProperty("url").AsString() == subscriptionUrl)
            isSubscriptionDownloadStatusReceived = true;
        });
      }
      auto subscription = filterEngine->GetSubscription(subscriptionUrl);
      EXPECT_EQ(0u, subscription.GetProperty("filters").AsList().size()) << subscriptionUrl;
      EXPECT_TRUE(subscription.GetProperty("downloadStatus").IsNull()) << subscriptionUrl;
      subscription.UpdateFilters();

      // Since currently the check is called from implemenation of web request
      // they have to been firstly scheduled, namely before processing of
      // 'is subscription download allowed' callbacks;
      DelayedTimer::ProcessImmediateTimers(timerTasks);

      for (const auto& isSubscriptionDowloadAllowedCallback : isSubscriptionDowloadAllowedCallbacks)
      {
        isSubscriptionDowloadAllowedCallback(isConnectionAllowed);
      }
      isSubscriptionDowloadAllowedCallbacks.clear();

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
}

TEST_F(FilterEngineTest, FilterCreation)
{
  AdblockPlus::Filter filter1 = filterEngine->GetFilter("foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, filter1.GetType());
  AdblockPlus::Filter filter2 = filterEngine->GetFilter("@@foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter2.GetType());
  AdblockPlus::Filter filter3 = filterEngine->GetFilter("example.com##foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE, filter3.GetType());
  AdblockPlus::Filter filter4 = filterEngine->GetFilter("example.com#@#foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION, filter4.GetType());
  AdblockPlus::Filter filter5 = filterEngine->GetFilter("  foo  ");
  ASSERT_EQ(filter1, filter5);
}

TEST_F(FilterEngineTest, FilterProperties)
{
  AdblockPlus::Filter filter = filterEngine->GetFilter("foo");

  ASSERT_TRUE(filter.GetProperty("stringFoo").IsUndefined());
  ASSERT_TRUE(filter.GetProperty("intFoo").IsUndefined());
  ASSERT_TRUE(filter.GetProperty("boolFoo").IsUndefined());

  filter.SetProperty("stringFoo", "y");
  filter.SetProperty("intFoo", 24);
  filter.SetProperty("boolFoo", true);
  ASSERT_EQ("y", filter.GetProperty("stringFoo").AsString());
  ASSERT_EQ(24, filter.GetProperty("intFoo").AsInt());
  ASSERT_TRUE(filter.GetProperty("boolFoo").AsBool());
}

TEST_F(FilterEngineTest, AddRemoveFilters)
{
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  AdblockPlus::Filter filter = filterEngine->GetFilter("foo");
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
  filter.AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine->GetListedFilters()[0]);
  ASSERT_TRUE(filter.IsListed());
  filter.AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(filter, filterEngine->GetListedFilters()[0]);
  ASSERT_TRUE(filter.IsListed());
  filter.RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
  filter.RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter.IsListed());
}

TEST_F(FilterEngineTest, SubscriptionProperties)
{
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");

  ASSERT_TRUE(subscription.GetProperty("stringFoo").IsUndefined());
  ASSERT_TRUE(subscription.GetProperty("intFoo").IsUndefined());
  ASSERT_TRUE(subscription.GetProperty("boolFoo").IsUndefined());

  subscription.SetProperty("stringFoo", "y");
  subscription.SetProperty("intFoo", 24);
  subscription.SetProperty("boolFoo", true);
  ASSERT_EQ("y", subscription.GetProperty("stringFoo").AsString());
  ASSERT_EQ(24, subscription.GetProperty("intFoo").AsInt());
  ASSERT_TRUE(subscription.GetProperty("boolFoo").AsBool());
}

TEST_F(FilterEngineTest, AddedSubscriptionIsEnabled)
{
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");
  EXPECT_FALSE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, DisablingSubscriptionDisablesItAndFiresEvent)
{
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");
  int eventFiredCounter = 0;
  filterEngine->SetFilterChangeCallback([&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject)
  {
    if (eventName != "subscription.disabled" || subscriptionObject.GetProperty("url").AsString() != "foo")
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
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");
  EXPECT_FALSE(subscription.IsDisabled());
  subscription.SetDisabled(true);
  EXPECT_TRUE(subscription.IsDisabled());

  int eventFiredCounter = 0;
  filterEngine->SetFilterChangeCallback([&eventFiredCounter](const std::string& eventName, JsValue&& subscriptionObject)
  {
    if (eventName != "subscription.disabled" || subscriptionObject.GetProperty("url").AsString() != "foo")
      return;
    ++eventFiredCounter;
  });
  subscription.SetDisabled(false);
  EXPECT_EQ(1, eventFiredCounter);
  EXPECT_FALSE(subscription.IsDisabled());
}

TEST_F(FilterEngineTest, AddRemoveSubscriptions)
{
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
  subscription.AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine->GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription.IsListed());
  subscription.AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(subscription, filterEngine->GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription.IsListed());
  subscription.RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
  subscription.RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription.IsListed());
}

TEST_F(FilterEngineTest, SubscriptionUpdates)
{
  AdblockPlus::Subscription subscription = filterEngine->GetSubscription("foo");
  ASSERT_FALSE(subscription.IsUpdating());
  subscription.UpdateFilters();
}

TEST_F(FilterEngineTest, Matches)
{
  filterEngine->GetFilter("adbanner.gif").AddToList();
  filterEngine->GetFilter("@@notbanner.gif").AddToList();
  filterEngine->GetFilter("tpbanner.gif$third-party").AddToList();
  filterEngine->GetFilter("fpbanner.gif$~third-party").AddToList();
  filterEngine->GetFilter("combanner.gif$domain=example.com").AddToList();
  filterEngine->GetFilter("orgbanner.gif$domain=~example.com").AddToList();

  AdblockPlus::FilterPtr match1 = filterEngine->Matches("http://example.org/foobar.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_FALSE(match1);

  AdblockPlus::FilterPtr match2 = filterEngine->Matches("http://example.org/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match2->GetType());

  AdblockPlus::FilterPtr match3 = filterEngine->Matches("http://example.org/notbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match3->GetType());

  AdblockPlus::FilterPtr match4 = filterEngine->Matches("http://example.org/notbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "");
  ASSERT_TRUE(match4);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match4->GetType());

  AdblockPlus::FilterPtr match5 = filterEngine->Matches("http://example.org/tpbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_FALSE(match5);

  AdblockPlus::FilterPtr match6 = filterEngine->Matches("http://example.org/fpbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_TRUE(match6);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match6->GetType());

  AdblockPlus::FilterPtr match7 = filterEngine->Matches("http://example.org/tpbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_TRUE(match7);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match7->GetType());

  AdblockPlus::FilterPtr match8 = filterEngine->Matches("http://example.org/fpbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_FALSE(match8);

  AdblockPlus::FilterPtr match9 = filterEngine->Matches("http://example.org/combanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_TRUE(match9);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match9->GetType());

  AdblockPlus::FilterPtr match10 = filterEngine->Matches("http://example.org/combanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_FALSE(match10);

  AdblockPlus::FilterPtr match11 = filterEngine->Matches("http://example.org/orgbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.com/");
  ASSERT_FALSE(match11);

  AdblockPlus::FilterPtr match12 = filterEngine->Matches("http://example.org/orgbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, "http://example.org/");
  ASSERT_TRUE(match12);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match12->GetType());
}

TEST_F(FilterEngineTest, MatchesOnWhitelistedDomain)
{
  filterEngine->GetFilter("adbanner.gif").AddToList();
  filterEngine->GetFilter("@@||example.org^$document").AddToList();

  AdblockPlus::FilterPtr match1 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          "http://example.com/");
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  AdblockPlus::FilterPtr match2 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          "http://example.org/");
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());
}

TEST_F(FilterEngineTest, MatchesWithContentTypeMask)
{
  filterEngine->GetFilter("adbanner.gif.js$script,image").AddToList();
  filterEngine->GetFilter("@@notbanner.gif").AddToList();
  filterEngine->GetFilter("blockme").AddToList();
  filterEngine->GetFilter("@@||example.doc^$document").AddToList();

  EXPECT_FALSE(filterEngine->Matches("http://example.org/foobar.gif",
    AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE, ""))
    << "another url should not match";

  EXPECT_FALSE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    /*mask*/ 0, "")) << "zero mask should not match (filter with some options)";

  EXPECT_FALSE(filterEngine->Matches("http://example.xxx/blockme",
    /*mask*/ 0, "")) << "zero mask should not match (filter without any option)";

  EXPECT_FALSE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::FilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one arbitrary flag in mask should not match";

  EXPECT_TRUE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::FilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one of flags in mask should match";

  EXPECT_TRUE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::FilterEngine::CONTENT_TYPE_SCRIPT, ""))
    << "both flags in mask should match";

  EXPECT_TRUE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE |
    AdblockPlus::FilterEngine::CONTENT_TYPE_SCRIPT |
    AdblockPlus::FilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "both flags with another flag in mask should match";

  EXPECT_TRUE(filterEngine->Matches("http://example.org/adbanner.gif.js",
    AdblockPlus::FilterEngine::CONTENT_TYPE_SCRIPT |
    AdblockPlus::FilterEngine::CONTENT_TYPE_OBJECT, ""))
    << "one of flags in mask should match";

  {
    AdblockPlus::FilterPtr filter;
    ASSERT_TRUE(filter = filterEngine->Matches("http://child.any/blockme",
      AdblockPlus::FilterEngine::CONTENT_TYPE_SCRIPT |
      AdblockPlus::FilterEngine::CONTENT_TYPE_OBJECT, "http://example.doc"))
      << "non-zero mask should match on whitelisted document";

    EXPECT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter->GetType());
  }

  {
    AdblockPlus::FilterPtr filter;
    ASSERT_TRUE(filter = filterEngine->Matches("http://example.doc/blockme",
      /*mask*/0, "http://example.doc"))
      << "zero mask should match when document is whitelisted";

    EXPECT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter->GetType());
  }
}

TEST_F(FilterEngineTest, MatchesNestedFrameRequest)
{
  filterEngine->GetFilter("adbanner.gif").AddToList();
  filterEngine->GetFilter("@@adbanner.gif$domain=example.org").AddToList();

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://ads.com/frame/");
  documentUrls1.push_back("http://example.com/");
  AdblockPlus::FilterPtr match1 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls1);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  std::vector<std::string> documentUrls2;
  documentUrls2.push_back("http://ads.com/frame/");
  documentUrls2.push_back("http://example.org/");
  AdblockPlus::FilterPtr match2 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls2);
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());

  std::vector<std::string> documentUrls3;
  documentUrls3.push_back("http://example.org/");
  documentUrls3.push_back("http://ads.com/frame/");
  AdblockPlus::FilterPtr match3 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls3);
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match3->GetType());
}

TEST_F(FilterEngineTest, MatchesNestedFrameOnWhitelistedDomain)
{
  filterEngine->GetFilter("adbanner.gif").AddToList();
  filterEngine->GetFilter("@@||example.org^$document,domain=ads.com").AddToList();

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://ads.com/frame/");
  documentUrls1.push_back("http://example.com/");
  AdblockPlus::FilterPtr match1 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls1);
  ASSERT_TRUE(match1);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match1->GetType());

  std::vector<std::string> documentUrls2;
  documentUrls2.push_back("http://ads.com/frame/");
  documentUrls2.push_back("http://example.org/");
  AdblockPlus::FilterPtr match2 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls2);
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match2->GetType());

  std::vector<std::string> documentUrls3;
  documentUrls3.push_back("http://example.org/");
  AdblockPlus::FilterPtr match3 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls3);
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match3->GetType());

  std::vector<std::string> documentUrls4;
  documentUrls4.push_back("http://example.org/");
  documentUrls4.push_back("http://ads.com/frame/");
  AdblockPlus::FilterPtr match4 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls4);
  ASSERT_TRUE(match4);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match4->GetType());

  std::vector<std::string> documentUrls5;
  documentUrls5.push_back("http://ads.com/frame/");
  documentUrls5.push_back("http://example.org/");
  documentUrls5.push_back("http://example.com/");
  AdblockPlus::FilterPtr match5 =
    filterEngine->Matches("http://ads.com/adbanner.gif", AdblockPlus::FilterEngine::CONTENT_TYPE_IMAGE,
                          documentUrls5);
  ASSERT_TRUE(match5);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match5->GetType());
}

TEST_F(FilterEngineTest, FirstRunFlag)
{
  ASSERT_FALSE(filterEngine->IsFirstRun());
}

TEST_F(FilterEngineTestNoData, FirstRunFlag)
{
  ASSERT_TRUE(filterEngine->IsFirstRun());
}

TEST_F(FilterEngineTest, SetRemoveFilterChangeCallback)
{
  int timesCalled = 0;
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  filterEngine->SetFilterChangeCallback([&timesCalled](const std::string&, AdblockPlus::JsValue&&)
  {
    timesCalled++;
  });
  filterEngine->GetFilter("foo").AddToList();
  EXPECT_EQ(1, timesCalled);

  filterEngine->RemoveFilterChangeCallback();
  filterEngine->GetFilter("foo").RemoveFromList();
  EXPECT_EQ(1, timesCalled);
}

TEST_F(UpdaterTest, SetRemoveUpdateAvailableCallback)
{
  mockWebRequest->response.status = 0;
  mockWebRequest->response.responseStatus = 200;
  mockWebRequest->response.responseText = "\
{\
  \"test\": {\
    \"version\": \"1.0.2\",\
    \"url\": \"https://downloads.adblockplus.org/test-1.0.2.tar.gz?update\"\
  }\
}";

  int timesCalled = 0;
  filterEngine->SetUpdateAvailableCallback([&timesCalled](const std::string&)->void
  {
    ++timesCalled;
  });
  filterEngine->ForceUpdateCheck();
  AdblockPlus::Sleep(100);
  EXPECT_EQ(1, timesCalled);

  filterEngine->RemoveUpdateAvailableCallback();
  filterEngine->ForceUpdateCheck();
  AdblockPlus::Sleep(100);
  EXPECT_EQ(1, timesCalled);
}

TEST_F(UpdaterTest, ForceUpdateCheck)
{
  mockWebRequest->response.status = 0;
  mockWebRequest->response.responseStatus = 200;
  mockWebRequest->response.responseText = "\
{\
  \"test\": {\
    \"version\": \"1.0.2\",\
    \"url\": \"https://downloads.adblockplus.org/test-1.0.2.tar.gz?update\"\
  }\
}";

  int called = 0; // 0 - not  called; 1 - once, no error; 2 - error
  filterEngine->ForceUpdateCheck([&called](const std::string& error)->void
  {
    called = error.empty() ? 1 : 2;
  });
  AdblockPlus::Sleep(100);
  EXPECT_EQ(1, called);
}

TEST_F(FilterEngineTest, DocumentWhitelisting)
{
  filterEngine->GetFilter("@@||example.org^$document").AddToList();
  filterEngine->GetFilter("@@||example.com^$document,domain=example.de").AddToList();

  ASSERT_TRUE(filterEngine->IsDocumentWhitelisted(
      "http://example.org",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine->IsDocumentWhitelisted(
      "http://example.co.uk",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine->IsDocumentWhitelisted(
      "http://example.com",
      std::vector<std::string>()));

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://example.de");

  ASSERT_TRUE(filterEngine->IsDocumentWhitelisted(
      "http://example.com",
      documentUrls1));

  ASSERT_FALSE(filterEngine->IsDocumentWhitelisted(
      "http://example.co.uk",
      documentUrls1));
}

TEST_F(FilterEngineTest, ElemhideWhitelisting)
{
  filterEngine->GetFilter("@@||example.org^$elemhide").AddToList();
  filterEngine->GetFilter("@@||example.com^$elemhide,domain=example.de").AddToList();

  ASSERT_TRUE(filterEngine->IsElemhideWhitelisted(
      "http://example.org",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine->IsElemhideWhitelisted(
      "http://example.co.uk",
      std::vector<std::string>()));

  ASSERT_FALSE(filterEngine->IsElemhideWhitelisted(
      "http://example.com",
      std::vector<std::string>()));

  std::vector<std::string> documentUrls1;
  documentUrls1.push_back("http://example.de");

  ASSERT_TRUE(filterEngine->IsElemhideWhitelisted(
      "http://example.com",
      documentUrls1));

  ASSERT_FALSE(filterEngine->IsElemhideWhitelisted(
      "http://example.co.uk",
      documentUrls1));
}

TEST_F(FilterEngineWithFreshFolder, LangAndAASubscriptionsAreChosenOnFirstRun)
{
  AppInfo appInfo;
  appInfo.locale = "zh";
  const std::string langSubscriptionUrl = "https://easylist-downloads.adblockplus.org/easylistchina+easylist.txt";
  auto jsEngine = createJsEngine(appInfo);
  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);
  const auto subscriptions = filterEngine->GetListedSubscriptions();
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
  EXPECT_EQ(langSubscriptionUrl, langSubscription->GetProperty("url").AsString());
  EXPECT_TRUE(filterEngine->IsAAEnabled());
}

TEST_F(FilterEngineWithFreshFolder, DisableSubscriptionsAutoSelectOnFirstRun)
{
  auto jsEngine = createJsEngine();
  FilterEngine::CreationParameters createParams;
  createParams.preconfiguredPrefs.emplace("first_run_subscription_auto_select", jsEngine->NewValue(false));
  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine, createParams);
  const auto subscriptions = filterEngine->GetListedSubscriptions();
  EXPECT_EQ(0u, subscriptions.size());
  EXPECT_FALSE(filterEngine->IsAAEnabled());
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

      {
        // no subscription (because of preconfigured prefs.json and patterns.ini),
        // though it should be enabled by default in a non-test environment, it's tested in
        // corresponding tests.
        const auto subscriptions = filterEngine->GetListedSubscriptions();
        EXPECT_EQ(0u, subscriptions.size()); // no any subscription including AA
        EXPECT_FALSE(filterEngine->IsAAEnabled());
      }
      if (otherSubscriptionsNumber == 1u)
      {
        auto subscription = filterEngine->GetSubscription(kOtherSubscriptionUrl);
        subscription.AddToList();
        const auto subscriptions = filterEngine->GetListedSubscriptions();
        ASSERT_EQ(1u, subscriptions.size());
        EXPECT_FALSE(subscriptions[0].IsAA());
        EXPECT_EQ(kOtherSubscriptionUrl, subscriptions[0].GetProperty("url").AsString());
      }
      if (isAASatusPresent(aaStatus))
      {
        filterEngine->SetAAEnabled(true); // add AA by enabling it
        if (aaStatus == AAStatus::disabled_present)
        {
          filterEngine->SetAAEnabled(false);
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
      if (aaStatus == AAStatus::enabled)
        EXPECT_TRUE(filterEngine->IsAAEnabled());
      else
        EXPECT_FALSE(filterEngine->IsAAEnabled());

      std::unique_ptr<Subscription> aaSubscription;
      std::unique_ptr<Subscription> otherSubscription;
      auto subscriptions = filterEngine->GetListedSubscriptions();
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

  INSTANTIATE_TEST_CASE_P(AA_ApiTests, Test,
    ::testing::Combine(::testing::ValuesIn(Test::VaryPossibleCases()), ::testing::Values<uint8_t>(0, 1)));

  TEST_P(Test, VaryPossibleCases) {
    const auto parameter = ::testing::get<0>(GetParam());
    uint8_t otherSubscriptionsNumber = ::testing::get<1>(GetParam());
    init(parameter.initialAAStatus, otherSubscriptionsNumber);

    if (parameter.action == Action::enable)
      filterEngine->SetAAEnabled(true);
    else if (parameter.action == Action::disable)
      filterEngine->SetAAEnabled(false);
    else if (parameter.action == Action::remove)
    {
      std::unique_ptr<Subscription> aaSubscription;
      for (auto& subscription : filterEngine->GetListedSubscriptions())
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
  createParams.isSubscriptionDowloadAllowedCallback = FilterEngine::IsConnectionAllowedAsyncCallback();
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
  EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, AllowingCallbackAllowsUpdating)
{
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
  EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_FALSE(capturedConnectionTypes[0].first);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, NotAllowingCallbackDoesNotAllowUpdating)
{
  isConnectionAllowed = false;
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_connection_error", subscription.GetProperty("downloadStatus").AsString());
  EXPECT_EQ(0u, subscription.GetProperty("filters").AsList().size());
  EXPECT_EQ(1u, capturedConnectionTypes.size());
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, PredefinedAllowedConnectionTypeIsPassedToCallback)
{
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs.insert(std::make_pair("allowed_connection_type",
    jsEngine->NewValue(predefinedAllowedConnectionType)));
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
  EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
}

TEST_F(FilterEngineIsSubscriptionDownloadAllowedTest, ConfiguredConnectionTypeIsPassedToCallback)
{
  // FilterEngine->RemoveSubscription is not usable here because subscriptions
  // are cached internally by URL. So, different URLs are used in diffirent
  // checks.
  {
    std::string predefinedAllowedConnectionType = "non-metered";
    createParams.preconfiguredPrefs.insert(std::make_pair(
      "allowed_connection_type", jsEngine->NewValue(predefinedAllowedConnectionType)));
    auto subscription = EnsureExampleSubscriptionAndForceUpdate();
    EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
    EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
  }
  capturedConnectionTypes.clear();
  {
    // set no value
    filterEngine->SetAllowedConnectionType(nullptr);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subA");
    EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
    EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_FALSE(capturedConnectionTypes[0].first);
    subscription.RemoveFromList();
  }
  capturedConnectionTypes.clear();
  {
    // set some value
    std::string testConnection = "test connection";
    filterEngine->SetAllowedConnectionType(&testConnection);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subB");
    EXPECT_EQ("synchronize_ok", subscription.GetProperty("downloadStatus").AsString());
    EXPECT_EQ(1u, subscription.GetProperty("filters").AsList().size());
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(testConnection, capturedConnectionTypes[0].second);
  }
}
