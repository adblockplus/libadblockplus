#include "BaseJsTest.h"

namespace
{
  typedef std::tr1::shared_ptr<AdblockPlus::FilterEngine> FilterEnginePtr;

  class FilterEngineTest : public BaseJsTest
  {
  protected:
    FilterEnginePtr filterEngine;

    void SetUp()
    {
      BaseJsTest::SetUp();
      // TODO: Don't use the default ErrorCallback/WebRequest
      jsEngine->SetErrorCallback(AdblockPlus::ErrorCallbackPtr(new AdblockPlus::DefaultErrorCallback));
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new AdblockPlus::DefaultWebRequest));
      filterEngine = FilterEnginePtr(new AdblockPlus::FilterEngine(jsEngine));
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
  filter->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(*filter, *filterEngine->GetListedFilters()[0]);
  filter->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedFilters().size());
  ASSERT_EQ(*filter, *filterEngine->GetListedFilters()[0]);
  filter->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
  filter->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedFilters().size());
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
  subscription->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(*subscription, *filterEngine->GetListedSubscriptions()[0]);
  subscription->AddToList();
  ASSERT_EQ(1u, filterEngine->GetListedSubscriptions().size());
  ASSERT_EQ(*subscription, *filterEngine->GetListedSubscriptions()[0]);
  subscription->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
  subscription->RemoveFromList();
  ASSERT_EQ(0u, filterEngine->GetListedSubscriptions().size());
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

  AdblockPlus::FilterPtr match1 = filterEngine->Matches("http://example.org/foobar.gif", "IMAGE", "");
  ASSERT_FALSE(match1);

  AdblockPlus::FilterPtr match2 = filterEngine->Matches("http://example.org/adbanner.gif", "IMAGE", "");
  ASSERT_TRUE(match2);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match2->GetType());

  AdblockPlus::FilterPtr match3 = filterEngine->Matches("http://example.org/notbanner.gif", "IMAGE", "");
  ASSERT_TRUE(match3);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match3->GetType());

  AdblockPlus::FilterPtr match4 = filterEngine->Matches("http://example.org/notbanner.gif", "IMAGE", "");
  ASSERT_TRUE(match4);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_EXCEPTION, match4->GetType());

  AdblockPlus::FilterPtr match5 = filterEngine->Matches("http://example.org/tpbanner.gif", "IMAGE", "http://example.org/");
  ASSERT_FALSE(match5);

  AdblockPlus::FilterPtr match6 = filterEngine->Matches("http://example.org/fpbanner.gif", "IMAGE", "http://example.org/");
  ASSERT_TRUE(match6);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match6->GetType());

  AdblockPlus::FilterPtr match7 = filterEngine->Matches("http://example.org/tpbanner.gif", "IMAGE", "http://example.com/");
  ASSERT_TRUE(match7);
  ASSERT_EQ(AdblockPlus::Filter::TYPE_BLOCKING, match6->GetType());

  AdblockPlus::FilterPtr match8 = filterEngine->Matches("http://example.org/fpbanner.gif", "IMAGE", "http://example.com/");
  ASSERT_FALSE(match8);
}
