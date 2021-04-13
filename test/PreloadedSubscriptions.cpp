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

#include <chrono>

#include "FilterEngineTest.h"

using namespace AdblockPlus;

class WrappingResourceLoader : public IResourceReader
{
public:
  typedef std::function<std::string(const std::string&)> Implementation;

  WrappingResourceLoader(Implementation callback) : impl(callback)
  {
  }

  void ReadPreloadedFilterList(const std::string& url,
                               const ReadCallback& doneCallback) const override
  {
    doneCallback(std::make_unique<StringPreloadedFilterResponse>(impl(url)));
  }

  Implementation impl;
};

class FilterEnginePreloadedSubscriptionsTest : public FilterEngineConfigurableTest
{
protected:
  int resourceLoaderCounter;

  AdblockPlus::IFilterEngine& ConfigureEngine(SynchronizationState syncState,
                                              const std::string& subscriptionUrl,
                                              const std::string& content)
  {
    resourceLoaderCounter = 0;
    PlatformFactory::CreationParameters params;
    params.resourceReader.reset(new WrappingResourceLoader(
        [subscriptionUrl, content, this](const std::string& url) -> std::string {
          if (url != subscriptionUrl)
            return "";

          ++resourceLoaderCounter;
          return content;
        }));
    this->filterList = content;
    return FilterEngineConfigurableTest::ConfigureEngine(
        AutoselectState::Disabled, syncState, std::move(params));
  }
};

TEST_F(FilterEnginePreloadedSubscriptionsTest, SubscribeForEnabledEngine)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content = "[Adblock Plus 2.0]\n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Enabled, url, content);

  EXPECT_EQ(0, webGETRequestCounter);
  EXPECT_EQ(0, resourceLoaderCounter);

  Subscription subscription = engine.GetSubscription(url);
  EXPECT_EQ(0, subscription.GetFilterCount());
  engine.AddSubscription(subscription);

  // should start download ASAP because Expires not set in content
  EXPECT_EQ(1, webGETRequestCounter);
  EXPECT_EQ(1, resourceLoaderCounter);
  EXPECT_EQ(1, subscription.GetFilterCount());
}

TEST_F(FilterEnginePreloadedSubscriptionsTest, Title)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content = "[Adblock Plus 2.0]\n!Title:Test title \n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Disabled, url, content);

  Subscription subscription = engine.GetSubscription(url);
  engine.AddSubscription(subscription);
  EXPECT_EQ("Test title ", subscription.GetTitle());
  EXPECT_EQ(1, subscription.GetFilterCount());
}

TEST_F(FilterEnginePreloadedSubscriptionsTest, Homepage)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content = "[Adblock Plus 2.0]\n !   Homepage:  \thttps://test.com\n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Disabled, url, content);

  Subscription subscription = engine.GetSubscription(url);
  engine.AddSubscription(subscription);
  EXPECT_EQ("https://test.com", subscription.GetHomepage());
}

TEST_F(FilterEnginePreloadedSubscriptionsTest, WrongHomepage)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content = "[Adblock Plus 2.0]\n!Homepage: test.com\n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Disabled, url, content);

  Subscription subscription = engine.GetSubscription(url);
  engine.AddSubscription(subscription);
  EXPECT_EQ("", subscription.GetHomepage());
}

TEST_F(FilterEnginePreloadedSubscriptionsTest, IgnoreRedirect)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content =
      "[Adblock Plus 2.0]\n!Redirect: https://other.com/subscription.txt\n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Disabled, url, content);

  Subscription subscription = engine.GetSubscription(url);
  engine.AddSubscription(subscription);
  EXPECT_EQ(url, subscription.GetUrl());
}

TEST_F(FilterEnginePreloadedSubscriptionsTest, Expires)
{
  std::string url = "https://test.com/subscription.txt";
  std::string content = "[Adblock Plus 2.0]\n!Expires: 5 h\n||example.com";
  auto& engine = ConfigureEngine(SynchronizationState::Enabled, url, content);

  Subscription subscription = engine.GetSubscription(url);
  engine.AddSubscription(subscription);
  EXPECT_EQ(1, resourceLoaderCounter);
  // should not start download because Expires is valid
  EXPECT_EQ(0, webGETRequestCounter);

  int period = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(5)).count();
  int softExpiration = GetJsEngine()
                           .Evaluate("API.getSubscriptionFromUrl('" + url +
                                     "').softExpiration - (Date.now() / 1000)")
                           .AsInt();
  int hardExpiration =
      GetJsEngine()
          .Evaluate("API.getSubscriptionFromUrl('" + url + "').expires - (Date.now() / 1000)")
          .AsInt();

  // processExpirationInterval can change value up to 20%
  EXPECT_NEAR(period, softExpiration, period / 5 + 1);
  // processExpirationInterval will set hard expiration to 2 x period
  EXPECT_NEAR(2 * period, hardExpiration, 1);
}
