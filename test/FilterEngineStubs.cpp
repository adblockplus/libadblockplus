#include <AdblockPlus.h>
#include <gtest/gtest.h>

TEST(FilterEngineStubsTest, FilterCreation)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);

  AdblockPlus::Filter& filter1 = filterEngine.GetFilter("foo");
  ASSERT_EQ(filter1.GetProperty("type", -1), AdblockPlus::Filter::TYPE_BLOCKING);
  AdblockPlus::Filter& filter2 = filterEngine.GetFilter("@@foo");
  ASSERT_EQ(filter2.GetProperty("type", -1), AdblockPlus::Filter::TYPE_EXCEPTION);
  AdblockPlus::Filter& filter3 = filterEngine.GetFilter("example.com##foo");
  ASSERT_EQ(filter3.GetProperty("type", -1), AdblockPlus::Filter::TYPE_ELEMHIDE);
  AdblockPlus::Filter& filter4 = filterEngine.GetFilter("example.com#@#foo");
  ASSERT_EQ(filter4.GetProperty("type", -1), AdblockPlus::Filter::TYPE_ELEMHIDE_EXCEPTION);
  AdblockPlus::Filter& filter5 = filterEngine.GetFilter("  foo  ");
  ASSERT_EQ(&filter5, &filter1);
}

TEST(FilterEngineStubsTest, FilterProperties)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  AdblockPlus::Filter& filter = filterEngine.GetFilter("foo");

  ASSERT_EQ(filter.GetProperty("stringFoo", "x"), "x");
  ASSERT_EQ(filter.GetProperty("intFoo", 42), 42);
  ASSERT_EQ(filter.GetProperty("boolFoo", false), false);

  filter.SetProperty("stringFoo", "y");
  filter.SetProperty("intFoo", 24);
  filter.SetProperty("boolFoo", true);
  ASSERT_EQ(filter.GetProperty("stringFoo", "x"), "y");
  ASSERT_EQ(filter.GetProperty("intFoo", 42), 24);
  ASSERT_EQ(filter.GetProperty("boolFoo", false), true);
}

TEST(FilterEngineStubsTest, AddRemoveFilters)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 0u);
  AdblockPlus::Filter& filter = filterEngine.GetFilter("foo");
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 0u);
  filter.AddToList();
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 1u);
  ASSERT_EQ(filterEngine.GetListedFilters()[0].get(), &filter);
  filter.AddToList();
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 1u);
  ASSERT_EQ(filterEngine.GetListedFilters()[0].get(), &filter);
  filter.RemoveFromList();
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 0u);
  filter.RemoveFromList();
  ASSERT_EQ(filterEngine.GetListedFilters().size(), 0u);
}

TEST(FilterEngineStubsTest, SubscriptionProperties)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  AdblockPlus::Subscription& subscription = filterEngine.GetSubscription("foo");

  ASSERT_EQ(subscription.GetProperty("stringFoo", "x"), "x");
  ASSERT_EQ(subscription.GetProperty("intFoo", 42), 42);
  ASSERT_EQ(subscription.GetProperty("boolFoo", false), false);

  subscription.SetProperty("stringFoo", "y");
  subscription.SetProperty("intFoo", 24);
  subscription.SetProperty("boolFoo", true);
  ASSERT_EQ(subscription.GetProperty("stringFoo", "x"), "y");
  ASSERT_EQ(subscription.GetProperty("intFoo", 42), 24);
  ASSERT_EQ(subscription.GetProperty("boolFoo", false), true);
}

TEST(FilterEngineStubsTest, AddRemoveSubscriptions)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
  AdblockPlus::Subscription& subscription = filterEngine.GetSubscription("foo");
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
  subscription.AddToList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 1u);
  ASSERT_EQ(filterEngine.GetListedSubscriptions()[0].get(), &subscription);
  subscription.AddToList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 1u);
  ASSERT_EQ(filterEngine.GetListedSubscriptions()[0].get(), &subscription);
  subscription.RemoveFromList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
  subscription.RemoveFromList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
}

TEST(FilterEngineStubsTest, Matches)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  AdblockPlus::Subscription& subscription = filterEngine.GetSubscription("foo");
  subscription.AddToList();

  AdblockPlus::FilterPtr match1 = filterEngine.Matches("http://example.org", "", "");
  ASSERT_FALSE(match1);

  AdblockPlus::FilterPtr match2 = filterEngine.Matches("http://example.org/adbanner.gif", "", "");
  ASSERT_TRUE(match2);
  ASSERT_EQ(match2->GetProperty("type", -1), AdblockPlus::Filter::TYPE_BLOCKING);

  AdblockPlus::FilterPtr match3 = filterEngine.Matches("http://example.org/notbanner.gif", "", "");
  ASSERT_TRUE(match3);
  ASSERT_EQ(match3->GetProperty("type", -1), AdblockPlus::Filter::TYPE_EXCEPTION);

  AdblockPlus::FilterPtr match4 = filterEngine.Matches("http://example.org/notbanner.gif", "", "");
  ASSERT_TRUE(match4);
  ASSERT_EQ(match4->GetProperty("type", -1), AdblockPlus::Filter::TYPE_EXCEPTION);
}
