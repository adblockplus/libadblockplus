/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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

TEST(AppInfoJsObjectTest, AllProperties)
{
  AdblockPlus::AppInfo appInfo;
  appInfo.id = "1";
  appInfo.version = "2";
  appInfo.name = "4";
  appInfo.platform = "5";
  AdblockPlus::JsEnginePtr jsEngine(AdblockPlus::JsEngine::New(appInfo));
  ASSERT_EQ("1", jsEngine->Evaluate("_appInfo.id")->AsString());
  ASSERT_EQ("2", jsEngine->Evaluate("_appInfo.version")->AsString());
  ASSERT_EQ("4", jsEngine->Evaluate("_appInfo.name")->AsString());
  ASSERT_EQ("5", jsEngine->Evaluate("_appInfo.platform")->AsString());
}
