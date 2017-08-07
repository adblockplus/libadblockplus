/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2017 eyeo GmbH
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

using namespace AdblockPlus;

namespace
{
  typedef std::shared_ptr<AdblockPlus::FilterEngine> FilterEnginePtr;

  class TestFileSystem : public LazyFileSystem
  {
  public:
    IOBuffer prefsContents;

    void Read(const std::string& path, const ReadCallback& callback) const override
    {
      scheduler([this, path, callback]
      {
        if (path == "prefs.json" && !prefsContents.empty())
        {
          callback(IOBuffer(prefsContents.cbegin(), prefsContents.cend()), "");
          return;
        }

        LazyFileSystem::Read(path, callback);
      });
    }

    void Write(const std::string& path, const IOBuffer& content, const Callback& callback) override
    {
      scheduler([this, path, content, callback]
      {
        if (path == "prefs.json")
        {
          prefsContents = content;
          callback("");
        }
      });
    }

    void Stat(const std::string& path, const StatCallback& callback) const override
    {
      scheduler([this, path, callback]
      {
        if (path == "prefs.json")
        {
          StatResult result;
          result.exists = result.isFile = !prefsContents.empty();
          callback(result, "");
          return;
        }

        LazyFileSystem::Stat(path, callback);
      });
    }
  };

  class PrefsTest : public ::testing::Test
  {
  protected:
    std::unique_ptr<Platform> platform;
    std::shared_ptr<TestFileSystem> fileSystem;

    void SetUp()
    {
      fileSystem = std::make_shared<TestFileSystem>();
      ResetPlatform();
    }

    void ResetPlatform()
    {
      ThrowingPlatformCreationParameters platformParams;
      platformParams.fileSystem = fileSystem;
      platformParams.webRequest.reset(new NoopWebRequest());
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer.reset(new NoopTimer());
      platform.reset(new Platform(std::move(platformParams)));
    }

    FilterEnginePtr CreateFilterEngine(const AdblockPlus::FilterEngine::Prefs& preconfiguredPrefs =
      AdblockPlus::FilterEngine::Prefs())
    {
      AdblockPlus::FilterEngine::CreationParameters createParams;
      createParams.preconfiguredPrefs = preconfiguredPrefs;
      return ::CreateFilterEngine(*fileSystem, *platform, createParams);
    }
  };
}

TEST_F(PrefsTest, PrefsGetSet)
{
  auto filterEngine = CreateFilterEngine();
  ASSERT_EQ("patterns.ini", filterEngine->GetPref("patternsfile").AsString());
  ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval").AsInt());
  ASSERT_TRUE(filterEngine->GetPref("subscriptions_autoupdate").AsBool());
  ASSERT_TRUE(filterEngine->GetPref("foobar").IsUndefined());

  ASSERT_ANY_THROW(filterEngine->SetPref("patternsfile", platform->GetJsEngine()->NewValue(0)));
  ASSERT_ANY_THROW(filterEngine->SetPref("patternsbackupinterval", platform->GetJsEngine()->NewValue(true)));
  ASSERT_ANY_THROW(filterEngine->SetPref("subscriptions_autoupdate", platform->GetJsEngine()->NewValue("foo")));

  filterEngine->SetPref("patternsfile", platform->GetJsEngine()->NewValue("filters.ini"));
  filterEngine->SetPref("patternsbackupinterval", platform->GetJsEngine()->NewValue(48));
  filterEngine->SetPref("subscriptions_autoupdate", platform->GetJsEngine()->NewValue(false));

  ASSERT_EQ("filters.ini", filterEngine->GetPref("patternsfile").AsString());
  ASSERT_EQ(48, filterEngine->GetPref("patternsbackupinterval").AsInt());
  ASSERT_FALSE(filterEngine->GetPref("subscriptions_autoupdate").AsBool());
}

TEST_F(PrefsTest, PrefsPersist)
{
  {
    auto filterEngine = CreateFilterEngine();
    ASSERT_EQ("patterns.ini", filterEngine->GetPref("patternsfile").AsString());
    ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval").AsInt());
    ASSERT_TRUE(filterEngine->GetPref("subscriptions_autoupdate").AsBool());

    filterEngine->SetPref("patternsfile", platform->GetJsEngine()->NewValue("filters.ini"));
    filterEngine->SetPref("patternsbackupinterval", platform->GetJsEngine()->NewValue(48));
    filterEngine->SetPref("subscriptions_autoupdate", platform->GetJsEngine()->NewValue(false));
  }
  ASSERT_FALSE(fileSystem->prefsContents.empty());

  {
    ResetPlatform();
    auto filterEngine = CreateFilterEngine();
    ASSERT_EQ("filters.ini", filterEngine->GetPref("patternsfile").AsString());
    ASSERT_EQ(48, filterEngine->GetPref("patternsbackupinterval").AsInt());
    ASSERT_FALSE(filterEngine->GetPref("subscriptions_autoupdate").AsBool());
  }
}

TEST_F(PrefsTest, UnknownPrefs)
{
  using IOBuffer = AdblockPlus::IFileSystem::IOBuffer;
  std::string content = "{\"foobar\":2, \"patternsbackupinterval\": 12}";
  fileSystem->prefsContents = IOBuffer(content.cbegin(), content.cend());
  auto filterEngine = CreateFilterEngine();
  ASSERT_TRUE(filterEngine->GetPref("foobar").IsUndefined());
  ASSERT_EQ(12, filterEngine->GetPref("patternsbackupinterval").AsInt());
}

TEST_F(PrefsTest, SyntaxFailure)
{
  using IOBuffer = AdblockPlus::IFileSystem::IOBuffer;
  std::string content = "{\"patternsbackupinterval\": 6, \"foo\"}";
  fileSystem->prefsContents = IOBuffer(content.cbegin(), content.cend());
  auto filterEngine = CreateFilterEngine();

  ASSERT_EQ(24, filterEngine->GetPref("patternsbackupinterval").AsInt());
}

TEST_F(PrefsTest, PreconfiguredPrefsPreconfigured)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("disable_auto_updates", platform->GetJsEngine()->NewValue(false));
  preconfiguredPrefs.emplace("suppress_first_run_page", platform->GetJsEngine()->NewValue(true));
  auto filterEngine = CreateFilterEngine(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("disable_auto_updates").IsBool());
  ASSERT_FALSE(filterEngine->GetPref("disable_auto_updates").AsBool());
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").IsBool());
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").AsBool());
}

TEST_F(PrefsTest, PreconfiguredPrefsUnsupported)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("unsupported_preconfig", platform->GetJsEngine()->NewValue(true));
  auto filterEngine = CreateFilterEngine(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine->GetPref("unsupported_preconfig").IsUndefined());
}

TEST_F(PrefsTest, PreconfiguredPrefsOverride)
{
  AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("suppress_first_run_page", platform->GetJsEngine()->NewValue(true));
  auto filterEngine = CreateFilterEngine(preconfiguredPrefs);

  filterEngine->SetPref("suppress_first_run_page", platform->GetJsEngine()->NewValue(false));
  ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").IsBool());
  ASSERT_FALSE(filterEngine->GetPref("suppress_first_run_page").AsBool());
}

TEST_F(PrefsTest, PrefsPersistWhenPreconfigured)
{
  {
    AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
    preconfiguredPrefs.emplace("suppress_first_run_page", platform->GetJsEngine()->NewValue(true));
    auto filterEngine = CreateFilterEngine(preconfiguredPrefs);

    ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").IsBool());
    ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").AsBool());
    filterEngine->SetPref("suppress_first_run_page", platform->GetJsEngine()->NewValue(false));
  }
  ASSERT_FALSE(fileSystem->prefsContents.empty());

  {
    ResetPlatform();
    AdblockPlus::FilterEngine::Prefs preconfiguredPrefs;
    preconfiguredPrefs.emplace("suppress_first_run_page", platform->GetJsEngine()->NewValue(true));
    auto filterEngine = CreateFilterEngine(preconfiguredPrefs);

    ASSERT_TRUE(filterEngine->GetPref("suppress_first_run_page").IsBool());
    ASSERT_FALSE(filterEngine->GetPref("suppress_first_run_page").AsBool());
  }
}
