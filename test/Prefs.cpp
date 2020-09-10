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

#include <sstream>

#include "../src/Thread.h"
#include "BaseJsTest.h"

using namespace AdblockPlus;

namespace
{
  class TestFileSystem : public LazyFileSystem
  {
    IOBuffer& prefsContents;
  public:
    explicit TestFileSystem(IOBuffer& prefsContent)
      : prefsContents(prefsContent)
    {
    }
    void Read(const std::string& fileName, const ReadCallback& callback, const Callback& errorCallback) const override
    {
      scheduler([this, fileName, callback, errorCallback]
      {
        if (fileName == "prefs.json" && !prefsContents.empty())
        {
          callback(IOBuffer(prefsContents.cbegin(), prefsContents.cend()));
          return;
        }
        LazyFileSystem::Read(fileName, callback, errorCallback);
      });
    }

    void Write(const std::string& fileName, const IOBuffer& content, const Callback& callback) override
    {
      scheduler([this, fileName, content, callback]
      {
        if (fileName == "prefs.json")
        {
          prefsContents = content;
          callback("");
        }
      });
    }

    void Stat(const std::string& fileName, const StatCallback& callback) const override
    {
      scheduler([this, fileName, callback]
      {
        if (fileName == "prefs.json")
        {
          StatResult result;
          result.exists = !prefsContents.empty();
          callback(result, "");
          return;
        }

        LazyFileSystem::Stat(fileName, callback);
      });
    }
  };

  class PrefsTest : public BaseJsTest
  {
    LazyFileSystem* fileSystem;
  protected:
    IFileSystem::IOBuffer prefsContent;

    void SetUp()
    {
      ResetPlatform();
    }

    void ResetPlatform()
    {
      ThrowingPlatformCreationParameters platformParams;
      platformParams.fileSystem.reset(fileSystem = new TestFileSystem(prefsContent));
      platformParams.webRequest.reset(new NoopWebRequest());
      platformParams.logSystem.reset(new LazyLogSystem());
      platformParams.timer.reset(new NoopTimer());
      platform.reset(new Platform(std::move(platformParams)));
    }

    IFilterEngine& CreateFilterEngine(const AdblockPlus::IFilterEngine::Prefs& preconfiguredPrefs =
      AdblockPlus::IFilterEngine::Prefs())
    {
      AdblockPlus::FilterEngineFactory::CreationParameters createParams;
      createParams.preconfiguredPrefs = preconfiguredPrefs;
      return ::CreateFilterEngine(*fileSystem, *platform, createParams);
    }
  };
}

TEST_F(PrefsTest, PrefsGetSet)
{
  auto& filterEngine = CreateFilterEngine();
  ASSERT_EQ(24, filterEngine.GetPref("patternsbackupinterval").AsInt());
  ASSERT_TRUE(filterEngine.GetPref("subscriptions_autoupdate").AsBool());
  ASSERT_TRUE(filterEngine.GetPref("foobar").IsUndefined());

  ASSERT_ANY_THROW(filterEngine.SetPref("patternsbackupinterval", GetJsEngine().NewValue(true)));
  ASSERT_ANY_THROW(filterEngine.SetPref("subscriptions_autoupdate", GetJsEngine().NewValue("foo")));

  filterEngine.SetPref("patternsbackupinterval", GetJsEngine().NewValue(48));
  filterEngine.SetPref("subscriptions_autoupdate", GetJsEngine().NewValue(false));

  ASSERT_EQ(48, filterEngine.GetPref("patternsbackupinterval").AsInt());
  ASSERT_FALSE(filterEngine.GetPref("subscriptions_autoupdate").AsBool());
}

TEST_F(PrefsTest, PrefsPersist)
{
  {
    auto& filterEngine = CreateFilterEngine();
    ASSERT_EQ(24, filterEngine.GetPref("patternsbackupinterval").AsInt());
    ASSERT_TRUE(filterEngine.GetPref("subscriptions_autoupdate").AsBool());

    filterEngine.SetPref("patternsbackupinterval", GetJsEngine().NewValue(48));
    filterEngine.SetPref("subscriptions_autoupdate", GetJsEngine().NewValue(false));
  }
  ASSERT_FALSE(prefsContent.empty());

  {
    ResetPlatform();
    auto& filterEngine = CreateFilterEngine();
    ASSERT_EQ(48, filterEngine.GetPref("patternsbackupinterval").AsInt());
    ASSERT_FALSE(filterEngine.GetPref("subscriptions_autoupdate").AsBool());
  }
}

TEST_F(PrefsTest, UnknownPrefs)
{
  using IOBuffer = AdblockPlus::IFileSystem::IOBuffer;
  std::string content = "{\"foobar\":2, \"patternsbackupinterval\": 12}";
  prefsContent = IOBuffer(content.cbegin(), content.cend());
  auto& filterEngine = CreateFilterEngine();
  ASSERT_TRUE(filterEngine.GetPref("foobar").IsUndefined());
  ASSERT_EQ(12, filterEngine.GetPref("patternsbackupinterval").AsInt());
}

TEST_F(PrefsTest, SyntaxFailure)
{
  using IOBuffer = AdblockPlus::IFileSystem::IOBuffer;
  std::string content = "{\"patternsbackupinterval\": 6, \"foo\"}";
  prefsContent = IOBuffer(content.cbegin(), content.cend());
  ASSERT_EQ(24, CreateFilterEngine().GetPref("patternsbackupinterval").AsInt());
}

TEST_F(PrefsTest, PreconfiguredPrefsPreconfigured)
{
  AdblockPlus::IFilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("suppress_first_run_page", GetJsEngine().NewValue(true));
  auto& filterEngine = CreateFilterEngine(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").IsBool());
  ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").AsBool());
}

TEST_F(PrefsTest, PreconfiguredPrefsUnsupported)
{
  AdblockPlus::IFilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("unsupported_preconfig", GetJsEngine().NewValue(true));
  auto& filterEngine = CreateFilterEngine(preconfiguredPrefs);

  ASSERT_TRUE(filterEngine.GetPref("unsupported_preconfig").IsUndefined());
}

TEST_F(PrefsTest, PreconfiguredPrefsOverride)
{
  AdblockPlus::IFilterEngine::Prefs preconfiguredPrefs;
  preconfiguredPrefs.emplace("suppress_first_run_page", GetJsEngine().NewValue(true));
  auto& filterEngine = CreateFilterEngine(preconfiguredPrefs);

  filterEngine.SetPref("suppress_first_run_page", GetJsEngine().NewValue(false));
  ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").IsBool());
  ASSERT_FALSE(filterEngine.GetPref("suppress_first_run_page").AsBool());
}

TEST_F(PrefsTest, PrefsPersistWhenPreconfigured)
{
  {
    AdblockPlus::IFilterEngine::Prefs preconfiguredPrefs;
    preconfiguredPrefs.emplace("suppress_first_run_page", GetJsEngine().NewValue(true));
    auto& filterEngine = CreateFilterEngine(preconfiguredPrefs);

    ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").IsBool());
    ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").AsBool());
    filterEngine.SetPref("suppress_first_run_page", GetJsEngine().NewValue(false));
  }
  ASSERT_FALSE(prefsContent.empty());

  {
    ResetPlatform();
    AdblockPlus::IFilterEngine::Prefs preconfiguredPrefs;
    preconfiguredPrefs.emplace("suppress_first_run_page", GetJsEngine().NewValue(true));
    auto& filterEngine = CreateFilterEngine(preconfiguredPrefs);

    ASSERT_TRUE(filterEngine.GetPref("suppress_first_run_page").IsBool());
    ASSERT_FALSE(filterEngine.GetPref("suppress_first_run_page").AsBool());
  }
}
