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

#include "../src/Utils.h"

#include <gtest/gtest.h>

using namespace AdblockPlus;

TEST(UtilsTest, TrimString)
{
  ASSERT_EQ("a b\tc\nd", Utils::TrimString(std::string(" \t    a b\tc\nd\r\n\t   ")));
}

TEST(UtilsTest, SplitString)
{
  auto res = Utils::SplitString(",a,,b,c,d,", ',');
  ASSERT_EQ(7u, res.size());
  ASSERT_EQ("", res[0]);
  ASSERT_EQ("a", res[1]);
  ASSERT_EQ("", res[2]);
  ASSERT_EQ("b", res[3]);
  ASSERT_EQ("c", res[4]);
  ASSERT_EQ("d", res[5]);
  ASSERT_EQ("", res[6]);
}