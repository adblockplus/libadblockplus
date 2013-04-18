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
