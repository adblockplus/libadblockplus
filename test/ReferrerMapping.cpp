/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
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

#include <AdblockPlus.h>
#include <gtest/gtest.h>

TEST(ReferrerMappingTest, EmptyReferrerChain)
{
  AdblockPlus::ReferrerMapping referrerMapping;
  std::vector<std::string> referrerChain =
    referrerMapping.BuildReferrerChain("first");
  ASSERT_EQ(1, referrerChain.size());
  ASSERT_EQ("first", referrerChain[0]);
}

TEST(ReferrerMappingTest, TwoElementReferrerChain)
{
  AdblockPlus::ReferrerMapping referrerMapping;
  referrerMapping.Add("second", "first");
  std::vector<std::string> referrerChain =
    referrerMapping.BuildReferrerChain("second");
  ASSERT_EQ(2, referrerChain.size());
  ASSERT_EQ("first", referrerChain[0]);
  ASSERT_EQ("second", referrerChain[1]);
}

TEST(ReferrerMappingTest, TenElementReferrerChain)
{
  AdblockPlus::ReferrerMapping referrerMapping;
  referrerMapping.Add("second", "first");
  referrerMapping.Add("third", "second");
  referrerMapping.Add("fourth", "third");
  referrerMapping.Add("fifth", "fourth");
  referrerMapping.Add("sixth", "fifth");
  referrerMapping.Add("seventh", "sixth");
  referrerMapping.Add("eighth", "seventh");
  referrerMapping.Add("ninth", "eighth");
  referrerMapping.Add("tenth", "ninth");
  std::vector<std::string> referrerChain =
    referrerMapping.BuildReferrerChain("tenth");
  ASSERT_EQ(10, referrerChain.size());
  ASSERT_EQ("first", referrerChain[0]);
  ASSERT_EQ("second", referrerChain[1]);
  ASSERT_EQ("third", referrerChain[2]);
  ASSERT_EQ("fourth", referrerChain[3]);
  ASSERT_EQ("fifth", referrerChain[4]);
  ASSERT_EQ("sixth", referrerChain[5]);
  ASSERT_EQ("seventh", referrerChain[6]);
  ASSERT_EQ("eighth", referrerChain[7]);
  ASSERT_EQ("ninth", referrerChain[8]);
  ASSERT_EQ("tenth", referrerChain[9]);
}

TEST(ReferrerMappingTest, CacheOnlyFiveUrls)
{
  AdblockPlus::ReferrerMapping referrerMapping(5);
  referrerMapping.Add("second", "first");
  referrerMapping.Add("third", "second");
  referrerMapping.Add("fourth", "third");
  referrerMapping.Add("fifth", "fourth");
  referrerMapping.Add("sixth", "fifth");
  referrerMapping.Add("seventh", "sixth");
  std::vector<std::string> referrerChain =
    referrerMapping.BuildReferrerChain("seventh");
  ASSERT_EQ(6, referrerChain.size());
  ASSERT_EQ("second", referrerChain[0]);
  ASSERT_EQ("third", referrerChain[1]);
  ASSERT_EQ("fourth", referrerChain[2]);
  ASSERT_EQ("fifth", referrerChain[3]);
  ASSERT_EQ("sixth", referrerChain[4]);
  ASSERT_EQ("seventh", referrerChain[5]);
}
