#include <AdblockPlus.h>
#include <gtest/gtest.h>

TEST(FilterEngineStubsTest, AddRemove)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  ASSERT_EQ(filterEngine.GetSubscriptions().size(), 0u);
  AdblockPlus::Subscription subscription("foo", "bar");
  filterEngine.AddSubscription(subscription);
  ASSERT_EQ(filterEngine.GetSubscriptions().size(), 1u);
  filterEngine.RemoveSubscription(subscription);
  ASSERT_EQ(filterEngine.GetSubscriptions().size(), 0u);
}

TEST(FilterEngineStubsTest, Matches)
{
  AdblockPlus::JsEngine jsEngine(0, 0);
  AdblockPlus::FilterEngine filterEngine(jsEngine);
  filterEngine.AddSubscription(AdblockPlus::Subscription("foo", "bar"));
  ASSERT_FALSE(filterEngine.Matches("http://example.org", ""));
  ASSERT_TRUE(filterEngine.Matches("http://example.org/adbanner.gif", ""));
}
