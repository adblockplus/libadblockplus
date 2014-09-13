/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2014 Eyeo GmbH
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

namespace
{
  typedef std::tr1::shared_ptr<AdblockPlus::FilterEngine> FilterEnginePtr;

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
      filterEngine = FilterEnginePtr(new AdblockPlus::FilterEngine(jsEngine));
    }
  };

  typedef FilterEngineTestGeneric<LazyFileSystem, AdblockPlus::DefaultLogSystem> FilterEngineTest;
  typedef FilterEngineTestGeneric<VeryLazyFileSystem, LazyLogSystem> FilterEngineTestNoData;
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
