#include <AdblockPlus.h>
#include <gtest/gtest.h>

TEST(FilterEngineStubsTest, AddRemove)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
  AdblockPlus::Subscription& subscription = filterEngine.GetSubscription("foo");
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 0u);
  subscription.AddToList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 1u);
  subscription.AddToList();
  ASSERT_EQ(filterEngine.GetListedSubscriptions().size(), 1u);
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
  ASSERT_FALSE(filterEngine.Matches("http://example.org", "", ""));
  ASSERT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif", "", ""));
}
