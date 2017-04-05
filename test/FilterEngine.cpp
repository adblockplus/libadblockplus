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
  class FilterEngineTestGeneric : public BaseJsTest
  {
  protected:
    FilterEnginePtr filterEngine;

    void SetUp()
    {
      BaseJsTest::SetUp();
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new FileSystem));
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new LazyWebRequest));
      jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new LogSystem));
      filterEngine = AdblockPlus::FilterEngine::Create(jsEngine);
    }
  };

  typedef FilterEngineTestGeneric<LazyFileSystem, AdblockPlus::DefaultLogSystem> FilterEngineTest;
  typedef FilterEngineTestGeneric<VeryLazyFileSystem, LazyLogSystem> FilterEngineTestNoData;

  struct MockFilterChangeCallback
  {
    MockFilterChangeCallback(int& timesCalled) : timesCalled(timesCalled) {}

    void operator()(const std::string&, const AdblockPlus::JsValuePtr)
    {
      timesCalled++;
    }

  private:
    int& timesCalled;
  };

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

    MockWebRequest* mockWebRequest;
    FilterEnginePtr filterEngine;

    void SetUp()
    {
      AdblockPlus::AppInfo appInfo;
      appInfo.name = "test";
      appInfo.version = "1.0.1";
      AdblockPlus::JsEnginePtr jsEngine = CreateJsEngine(appInfo);
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem));
      mockWebRequest = new MockWebRequest;
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(mockWebRequest));
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
      auto jsEngine = JsEngine::New(appInfo);
      weakJsEngine = jsEngine;
      jsEngine->SetFileSystem(fileSystem);
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new LazyWebRequest()));
      jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new LazyLogSystem()));
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

  class FilterEngineIsAllowedConnectionTest : public BaseJsTest
  {
    class MockWebRequest : public LazyWebRequest
    {
    public:
      std::map</*beginning of url*/std::string, AdblockPlus::ServerResponse> responses;

      AdblockPlus::ServerResponse GET(const std::string& url,
        const AdblockPlus::HeaderList& requestHeaders) const
      {
        for (const auto& response : responses)
        {
          if (Utils::BeginsWith(url, response.first))
          {
            return response.second;
          }
        }
        return LazyWebRequest::GET(url, requestHeaders);
      }
    };
    class SyncStrings
    {
    public:
      void Add(const std::string* value)
      {
        std::lock_guard<std::mutex> lock(mutex);
        strings.emplace_back(!!value, value ? *value : "");
      }
      std::vector<std::pair<bool, std::string>> GetStrings() const
      {
        std::lock_guard<std::mutex> lock(mutex);
        return strings;
      }
      void Clear()
      {
        std::lock_guard<std::mutex> lock(mutex);
        strings.clear();
      }
    private:
      mutable std::mutex mutex;
      std::vector<std::pair<bool, std::string>> strings;
    };
  protected:
    MockWebRequest* webRequest;
    std::string subscriptionUrlPrefix;
    FilterEngine::CreationParameters createParams;
    SyncStrings capturedConnectionTypes;
    bool isConnectionAllowed;
    FilterEnginePtr filterEngine;

    struct
    {
      std::string url;
      std::mutex mutex;
      std::condition_variable cv;
    } downloadStatusChanged;

    void SetUp()
    {
      BaseJsTest::SetUp();
      jsEngine->SetFileSystem(AdblockPlus::FileSystemPtr(new LazyFileSystem()));
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(webRequest = new MockWebRequest()));
      jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new LazyLogSystem()));

      subscriptionUrlPrefix = "http://example";
      ServerResponse exampleSubscriptionResponse;
      exampleSubscriptionResponse.responseStatus = 200;
      exampleSubscriptionResponse.status = WebRequest::NS_OK;
      exampleSubscriptionResponse.responseText = "[Adblock Plus 2.0]\n||example.com";
      webRequest->responses.emplace(subscriptionUrlPrefix, exampleSubscriptionResponse);
      createParams.preconfiguredPrefs["first_run_subscription_auto_select"] = jsEngine->NewValue(false);
      isConnectionAllowed = true;
      createParams.isConnectionAllowedCallback = [this](const std::string* allowedConnectionType)->bool{
        capturedConnectionTypes.Add(allowedConnectionType);
        return isConnectionAllowed;
      };
      jsEngine->SetEventCallback("filterChange", [this](const JsValueList& params/*action, item*/)
      {
        ASSERT_EQ(2u, params.size());
        if (params[0]->AsString() == "subscription.downloadStatus")
        {
          {
            std::lock_guard<std::mutex> lock(downloadStatusChanged.mutex);
            downloadStatusChanged.url = params[1]->GetProperty("url")->AsString();
          }
          downloadStatusChanged.cv.notify_one();
        }
      });
    }

    SubscriptionPtr EnsureExampleSubscriptionAndForceUpdate(const std::string& apppendToUrl = std::string())
    {
      if (!filterEngine)
        filterEngine = FilterEngine::Create(jsEngine, createParams);
      auto subscriptionUrl = subscriptionUrlPrefix + apppendToUrl;
      auto subscription = filterEngine->GetSubscription(subscriptionUrl);
      EXPECT_EQ(0u, subscription->GetProperty("filters")->AsList().size()) << subscriptionUrl;
      EXPECT_TRUE(subscription->GetProperty("downloadStatus")->IsNull()) << subscriptionUrl;
      subscription->UpdateFilters();
      {
        std::unique_lock<std::mutex> lock(downloadStatusChanged.mutex);
        downloadStatusChanged.cv.wait_for(lock,
          /*don't block tests forever*/std::chrono::seconds(5),
          [this, subscriptionUrl]()->bool
        {
          return subscriptionUrl == downloadStatusChanged.url;
        });
        // Basically it's enough to wait only for downloadStatus although there
        // is still some JS code being executed. Any following attempt to work
        // with subscription object will result in execution of JS, which will
        // be blocked until finishing of currently running code.
      }
      return subscription;
    }
  };
}

TEST_F(FilterEngineTest, FilterCreation)
{
  AdblockPlus::FilterPtr filter1 = filterEngine->GetFilter("foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, filter1->GetType());
  AdblockPlus::FilterPtr filter2 = filterEngine->GetFilter("@@foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, filter2->GetType());
  AdblockPlus::FilterPtr filter3 = filterEngine->GetFilter("example.com##foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE, filter3->GetType());
  AdblockPlus::FilterPtr filter4 = filterEngine->GetFilter("example.com#@#foo");
  ASSERT_EQ(AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION, filter4->GetType());
  AdblockPlus::FilterPtr filter5 = filterEngine->GetFilter("  foo  ");
  ASSERT_EQ(*filter1, *filter5);
}

TEST_F(FilterEngineTest, FilterProperties)
{
  AdblockPlus::FilterPtr filter = filterEngine->GetFilter("foo");

  ASSERT_TRUE(filter->GetProperty("stringFoo")->IsUndefined());
  ASSERT_TRUE(filter->GetProperty("intFoo")->IsUndefined());
  ASSERT_TRUE(filter->GetProperty("boolFoo")->IsUndefined());

  filter->SetProperty("stringFoo", "y");
  filter->SetProperty("intFoo", 24);
  filter->SetProperty("boolFoo", true);
  ASSERT_EQ("y", filter->GetProperty("stringFoo")->AsString());
  ASSERT_EQ(24, filter->GetProperty("intFoo")->AsInt());
  ASSERT_TRUE(filter->GetProperty("boolFoo")->AsBool());
}

TEST_F(FilterEngineTest, AddRemoveFilters)
{
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  AdblockPlus::FilterPtr filter = filterEngine->GetFilter("foo");
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter->IsListed());
  filter->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(*filter, *filterEngine->GetListedFilters()[0]);
  ASSERT_TRUE(filter->IsListed());
  filter->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(*filter, *filterEngine->GetListedFilters()[0]);
  ASSERT_TRUE(filter->IsListed());
  filter->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter->IsListed());
  filter->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  ASSERT_FALSE(filter->IsListed());
}

TEST_F(FilterEngineTest, SubscriptionProperties)
{
  AdblockPlus::SubscriptionPtr subscription = filterEngine->GetSubscription("foo");

  ASSERT_TRUE(subscription->GetProperty("stringFoo")->IsUndefined());
  ASSERT_TRUE(subscription->GetProperty("intFoo")->IsUndefined());
  ASSERT_TRUE(subscription->GetProperty("boolFoo")->IsUndefined());

  subscription->SetProperty("stringFoo", "y");
  subscription->SetProperty("intFoo", 24);
  subscription->SetProperty("boolFoo", true);
  ASSERT_EQ("y", subscription->GetProperty("stringFoo")->AsString());
  ASSERT_EQ(24, subscription->GetProperty("intFoo")->AsInt());
  ASSERT_TRUE(subscription->GetProperty("boolFoo")->AsBool());
}

TEST_F(FilterEngineTest, AddRemoveSubscriptions)
{
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  AdblockPlus::SubscriptionPtr subscription = filterEngine->GetSubscription("foo");
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription->IsListed());
  subscription->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(*subscription, *filterEngine->GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription->IsListed());
  subscription->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(*subscription, *filterEngine->GetListedSubscriptions()[0]);
  ASSERT_TRUE(subscription->IsListed());
  subscription->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription->IsListed());
  subscription->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  ASSERT_FALSE(subscription->IsListed());
}

TEST_F(FilterEngineTest, SubscriptionUpdates)
{
  AdblockPlus::SubscriptionPtr subscription = filterEngine->GetSubscription("foo");
  ASSERT_FALSE(subscription->IsUpdating());
  subscription->UpdateFilters();
}

TEST_F(FilterEngineTest, Matches)
{
  filterEngine->GetFilter("adbanner.gif")->AddToList();
  filterEngine->GetFilter("@@notbanner.gif")->AddToList();
  filterEngine->GetFilter("tpbanner.gif$third-party")->AddToList();
  filterEngine->GetFilter("fpbanner.gif$~third-party")->AddToList();
  filterEngine->GetFilter("combanner.gif$domain=example.com")->AddToList();
  filterEngine->GetFilter("orgbanner.gif$domain=~example.com")->AddToList();

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
  filterEngine->GetFilter("adbanner.gif")->AddToList();
  filterEngine->GetFilter("@@||example.org^$document")->AddToList();

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
  filterEngine->GetFilter("adbanner.gif.js$script,image")->AddToList();
  filterEngine->GetFilter("@@notbanner.gif")->AddToList();
  filterEngine->GetFilter("blockme")->AddToList();
  filterEngine->GetFilter("@@||example.doc^$document")->AddToList();

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
  filterEngine->GetFilter("adbanner.gif")->AddToList();
  filterEngine->GetFilter("@@adbanner.gif$domain=example.org")->AddToList();

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
  filterEngine->GetFilter("adbanner.gif")->AddToList();
  filterEngine->GetFilter("@@||example.org^$document,domain=ads.com")->AddToList();

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
  MockFilterChangeCallback mockFilterChangeCallback(timesCalled);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  filterEngine->SetFilterChangeCallback(mockFilterChangeCallback);
  filterEngine->GetFilter("foo")->AddToList();
  EXPECT_EQ(1, timesCalled);

  filterEngine->RemoveFilterChangeCallback();
  filterEngine->GetFilter("foo")->RemoveFromList();
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
  filterEngine->GetFilter("@@||example.org^$document")->AddToList();
  filterEngine->GetFilter("@@||example.com^$document,domain=example.de")->AddToList();

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
  filterEngine->GetFilter("@@||example.org^$elemhide")->AddToList();
  filterEngine->GetFilter("@@||example.com^$elemhide,domain=example.de")->AddToList();

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
  const auto aaUrl = filterEngine->GetPref("subscriptions_exceptionsurl")->AsString();
  SubscriptionPtr aaSubscription;
  SubscriptionPtr langSubscription;
  if (subscriptions[0]->GetProperty("url")->AsString() == aaUrl)
  {
    aaSubscription = subscriptions[0];
    langSubscription = subscriptions[1];
  }
  else if (subscriptions[1]->GetProperty("url")->AsString() == aaUrl)
  {
    aaSubscription = subscriptions[1];
    langSubscription = subscriptions[0];
  }
  ASSERT_NE(nullptr, aaSubscription);
  ASSERT_NE(nullptr, langSubscription);
  EXPECT_EQ(aaUrl, aaSubscription->GetProperty("url")->AsString());
  EXPECT_EQ(langSubscriptionUrl, langSubscription->GetProperty("url")->AsString());
}

TEST_F(FilterEngineWithFreshFolder, DisableSubscriptionsAutoSelectOnFirstRun)
{
  auto jsEngine = createJsEngine();
  FilterEngine::CreationParameters createParams;
  createParams.preconfiguredPrefs["first_run_subscription_auto_select"] = jsEngine->NewValue(false);
  auto filterEngine = AdblockPlus::FilterEngine::Create(jsEngine, createParams);
  const auto subscriptions = filterEngine->GetListedSubscriptions();
  EXPECT_EQ(0u, subscriptions.size());
}

TEST_F(FilterEngineIsAllowedConnectionTest, AbsentCallbackAllowsUpdating)
{
  createParams.isConnectionAllowedCallback = FilterEngine::IsConnectionAllowedCallback();
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
  EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
}

TEST_F(FilterEngineIsAllowedConnectionTest, AllowingCallbackAllowsUpdating)
{
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
  EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
  auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_FALSE(capturedConnectionTypes[0].first);
}

TEST_F(FilterEngineIsAllowedConnectionTest, NotAllowingCallbackDoesNotAllowUpdating)
{
  isConnectionAllowed = false;
  // no stored allowed_connection_type preference
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_connection_error", subscription->GetProperty("downloadStatus")->AsString());
  EXPECT_EQ(0u, subscription->GetProperty("filters")->AsList().size());
  auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
  EXPECT_EQ(1u, capturedConnectionTypes.size());
}

TEST_F(FilterEngineIsAllowedConnectionTest, PredefinedAllowedConnectionTypeIsPassedToCallback)
{
  std::string predefinedAllowedConnectionType = "non-metered";
  createParams.preconfiguredPrefs["allowed_connection_type"] = jsEngine->NewValue(predefinedAllowedConnectionType);
  auto subscription = EnsureExampleSubscriptionAndForceUpdate();
  EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
  EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
  auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
  ASSERT_EQ(1u, capturedConnectionTypes.size());
  EXPECT_TRUE(capturedConnectionTypes[0].first);
  EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
}

TEST_F(FilterEngineIsAllowedConnectionTest, ConfiguredConnectionTypeIsPassedToCallback)
{
  // FilterEngine->RemoveSubscription is not usable here because subscriptions
  // are cached internally by URL. So, different URLs are used in diffirent
  // checks.
  {
    std::string predefinedAllowedConnectionType = "non-metered";
    createParams.preconfiguredPrefs["allowed_connection_type"] = jsEngine->NewValue(predefinedAllowedConnectionType);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate();
    EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
    EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
    auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(predefinedAllowedConnectionType, capturedConnectionTypes[0].second);
  }
  capturedConnectionTypes.Clear();
  {
    // set no value
    filterEngine->SetAllowedConnectionType(nullptr);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subA");
    EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
    EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
    auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_FALSE(capturedConnectionTypes[0].first);
    subscription->RemoveFromList();
    this->capturedConnectionTypes.Clear();
  }
  capturedConnectionTypes.Clear();
  {
    // set some value
    std::string testConnection = "test connection";
    filterEngine->SetAllowedConnectionType(&testConnection);
    auto subscription = EnsureExampleSubscriptionAndForceUpdate("subB");
    EXPECT_EQ("synchronize_ok", subscription->GetProperty("downloadStatus")->AsString());
    EXPECT_EQ(1u, subscription->GetProperty("filters")->AsList().size());
    auto capturedConnectionTypes = this->capturedConnectionTypes.GetStrings();
    ASSERT_EQ(1u, capturedConnectionTypes.size());
    EXPECT_TRUE(capturedConnectionTypes[0].first);
    EXPECT_EQ(testConnection, capturedConnectionTypes[0].second);
  }
}