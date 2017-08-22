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

#include <AdblockPlus.h>
#include <gtest/gtest.h>
#include "BaseJsTest.h"

TEST(AppInfoJsObjectTest, AllProperties)
{
  AdblockPlus::AppInfo appInfo;
  appInfo.version = "1";
  appInfo.name = "3";
  appInfo.application = "4";
  appInfo.applicationVersion = "5";
  appInfo.locale = "2";
  appInfo.developmentBuild = true;
  AdblockPlus::Platform platform{ThrowingPlatformCreationParameters()};
  platform.SetUpJsEngine(appInfo);
  ASSERT_EQ("1", platform.GetJsEngine().Evaluate("_appInfo.version").AsString());
  ASSERT_EQ("3", platform.GetJsEngine().Evaluate("_appInfo.name").AsString());
  ASSERT_EQ("4", platform.GetJsEngine().Evaluate("_appInfo.application").AsString());
  ASSERT_EQ("5", platform.GetJsEngine().Evaluate("_appInfo.applicationVersion").AsString());
  ASSERT_EQ("2", platform.GetJsEngine().Evaluate("_appInfo.locale").AsString());
  ASSERT_TRUE(platform.GetJsEngine().Evaluate("_appInfo.developmentBuild").AsBool());
}

TEST(AppInfoJsObjectTest, DefaultPropertyValues)
{
  AdblockPlus::Platform platform{ThrowingPlatformCreationParameters()};
  ASSERT_EQ("", platform.GetJsEngine().Evaluate("_appInfo.version").AsString());
  ASSERT_EQ("", platform.GetJsEngine().Evaluate("_appInfo.name").AsString());
  ASSERT_EQ("", platform.GetJsEngine().Evaluate("_appInfo.application").AsString());
  ASSERT_EQ("", platform.GetJsEngine().Evaluate("_appInfo.applicationVersion").AsString());
  ASSERT_EQ("", platform.GetJsEngine().Evaluate("_appInfo.locale").AsString());
  ASSERT_FALSE(platform.GetJsEngine().Evaluate("_appInfo.developmentBuild").AsBool());
}
