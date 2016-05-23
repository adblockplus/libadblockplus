/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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

#include <sstream>

#include "../src/Thread.h"
#include "BaseJsTest.h"

namespace
{
  typedef std::shared_ptr<AdblockPlus::FilterEngine> FilterEnginePtr;

  class TestFileSystem : public LazyFileSystem
  {
  public:
    std::string prefsContents;

    std::shared_ptr<std::istream> Read(const std::string& path) const
    {
      if (path == "prefs.json" && !prefsContents.empty())
        return std::shared_ptr<std::istream>(new std::istringstream(prefsContents));

      return LazyFileSystem::Read(path);
    }

    void Write(const std::string& path, std::shared_ptr<std::istream> content)
    {
      if (path == "prefs.json")
      {
        std::stringstream ss;
        ss << content->rdbuf();
        prefsContents = ss.str();
      }
      else
        LazyFileSystem::Write(path, content);
    }

    StatResult Stat(const std::string& path) const
    {
      if (path == "prefs.json")
      {
        StatResult result;
        result.exists = result.isFile = !prefsContents.empty();
        return result;
      }

      return LazyFileSystem::Stat(path);
    }
  };

  class PrefsTest : public ::testing::Test
  {
  protected:
    TestFileSystem* fileSystem;
    AdblockPlus::FileSystemPtr fileSystemPtr;
    AdblockPlus::JsEnginePtr jsEngine;
    FilterEnginePtr filterEngine;

    void SetUp()
    {
      fileSystem = new TestFileSystem();
      fileSystemPtr.reset(fileSystem);

      Reset();
    }

    void Reset(const AdblockPlus::FilterEngine::Prefs& preconfiguredPrefs =
               AdblockPlus::FilterEngine::Prefs())
    {
      jsEngine = CreateJsEngine();
      jsEngine->SetLogSystem(AdblockPlus::LogSystemPtr(new LazyLogSystem));
      jsEngine->SetFileSystem(fileSystemPtr);
      jsEngine->SetWebRequest(AdblockPlus::WebRequestPtr(new LazyWebRequest));

      filterEngine.reset(
        new AdblockPlus::FilterEngine(jsEngine, preconfiguredPrefs));
    }
  };
}

TEST_F(PrefsTest, PrefsGetSet)
{
  ASSERT_EQ("patterns.ini", filterEngine->GetPref("patternsfile")->AsString());
  ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval")->AsInt());
  ASSERT_TRUE(filterEngine->GetPref("subscriptions_autoupdate")->AsBool());
  ASSERT_TRUE(filterEngine->GetPref("foobar")->IsUndefined());

  ASSERT_ANY_THROW(filterEngine->SetPref("patternsfile", jsEngine->NewValue(0)));
  ASSERT_ANY_THROW(filterEngine->SetPref("patternsbackupinterval", jsEngine->NewValue(true)));
  ASSERT_ANY_THROW(filterEngine->SetPref("subscriptions_autoupdate", jsEngine->NewValue("foo")));

  filterEngine->SetPref("patternsfile", jsEngine->NewValue("filters.ini"));
  filterEngine->SetPref("patternsbackupinterval", jsEngine->NewValue(48));
  filterEngine->SetPref("subscriptions_autoupdate", jsEngine->NewValue(false));

  ASSERT_EQ("filters.ini", filterEngine->GetPref("patternsfile")->AsString());
  ASSERT_EQ(48, filterEngine->GetPref("patternsbackupinterval")->AsInt());
  ASSERT_FALSE(filterEngine->GetPref("subscriptions_autoupdate")->AsBool());
}

TEST_F(PrefsTest, PrefsPersist)
{
  ASSERT_EQ("patterns.ini", filterEngine->GetPref("patternsfile")->AsString());
  ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval")->AsInt());
  ASSERT_TRUE(filterEngine->GetPref("subscriptions_autoupdate")->AsBool());

  filterEngine->SetPref("patternsfile", jsEngine->NewValue("filters.ini"));
  filterEngine->SetPref("patternsbackupinterval", jsEngine->NewValue(48));
  filterEngine->SetPref("subscriptions_autoupdate", jsEngine->NewValue(false));

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(fileSystem->prefsContents.empty());

  Reset();

  ASSERT_EQ("filters.ini", filterEngine->GetPref("patternsfile")->AsString());
  ASSERT_EQ(48, filterEngine->GetPref("patternsbackupinterval")->AsInt());
  ASSERT_FALSE(filterEngine->GetPref("subscriptions_autoupdate")->AsBool());
}

TEST_F(PrefsTest, UnknownPrefs)
{
  fileSystem->prefsContents = "{\"foobar\":2, \"patternsbackupinterval\": 12}";
  Reset();

  ASSERT_TRUE(filterEngine->GetPref("foobar")->IsUndefined());
  ASSERT_EQ(12, filterEngine->GetPref("patternsbackupinterval")->AsInt());
}

TEST_F(PrefsTest, SyntaxFailure)
{
  fileSystem->prefsContents = "{\"patternsbackupinterval\": 6, \"foo\"}";
  Reset();

  ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval")->AsInt());
}

TEST_F(PrefsTest, PreconfiguredPrefsPreconfigured)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs["disable_auto_updates"] = jsEngine->NewValue(false);
  preconfiguredPrefs["suppress_first_run_page"] = jsEngine->NewValue(true);
  Reset(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("disable_auto_updates")->IsBool());
  ASSERT_FALSE(filterEngine->GetPref("disable_auto_updates")->AsBool());
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->IsBool());
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->AsBool());
}

TEST_F(PrefsTest, PreconfiguredPrefsUnsupported)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs["unsupported_preconfig"] = jsEngine->NewValue(true);
  Reset(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("unsupported_preconfig")->IsUndefined());
}

TEST_F(PrefsTest, PreconfiguredPrefsOverride)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs["suppress_first_run_page"] = jsEngine->NewValue(true);
  Reset(preconfiguredPrefs);

  filterEngine->SetPref("suppress_first_run_page", jsEngine->NewValue(false));
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->IsBool());
  ASSERT_FALSE(filterEngine->GetPref("suppress_first_run_page")->AsBool());
}

TEST_F(PrefsTest, PrefsPersistWhenPreconfigured)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs["suppress_first_run_page"] = jsEngine->NewValue(true);
  Reset(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->IsBool());
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->AsBool());
  filterEngine->SetPref("suppress_first_run_page", jsEngine->NewValue(false));

  AdblockPlus::Sleep(100);

  ASSERT_FALSE(fileSystem->prefsContents.empty());

  Reset(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page")->IsBool());
  ASSERT_FALSE(filterEngine->GetPref("suppress_first_run_page")->AsBool());
}