#include <fstream>
#include <gtest/gtest.h>

#include "../src/DefaultFileSystem.h"
#include "../src/DefaultPlatform.h"
#include "../src/JsEngine.h"
#include "../src/JsError.h"
#include "AdblockPlus.h"

class ReadOnlyFileSystem : public AdblockPlus::DefaultFileSystem
{
public:
  ReadOnlyFileSystem(AdblockPlus::IExecutor& executor, const std::string& basePath)
      : AdblockPlus::DefaultFileSystem(
            executor, std::make_unique<AdblockPlus::DefaultFileSystemSync>(basePath))
  {
  }

  void Write(const std::string& fileName, const IOBuffer& data, const Callback& callback) override
  {
    callback("");
  }

  void Move(const std::string& fromFileName,
            const std::string& toFileName,
            const Callback& callback) override
  {
    callback("");
  }

  void Remove(const std::string& fileName, const Callback& callback) override
  {
    callback("");
  }
};

class WebRequestNoResponce : public AdblockPlus::IWebRequest
{
public:
  void GET(const std::string& url,
           const AdblockPlus::HeaderList& requestHeaders,
           const RequestCallback& getCallback) override
  {
  }

  void HEAD(const std::string& url,
            const AdblockPlus::HeaderList& requestHeaders,
            const RequestCallback& requestCallback) override
  {
  }
};

enum class PopupBlockResult
{
  NO_RULE,
  BLOCK_RULE,
  ALLOW_RULE,
  DISABLED
};

class ElapsedTime
{
public:
  ElapsedTime()
  {
    start = std::chrono::steady_clock::now();
  }

  double Milliseconds() const
  {
    std::chrono::steady_clock::duration d = std::chrono::steady_clock::now() - start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  }

private:
  std::chrono::steady_clock::time_point start;
};

class HarnessTest : public ::testing::Test
{
protected:
  std::unique_ptr<AdblockPlus::Platform> platform;

  void SetUp() override
  {
    AdblockPlus::AppInfo appInfo;
    appInfo.version = "1.0";
    appInfo.name = "abppplayer";
    appInfo.application = "standalone";
    appInfo.applicationVersion = "1.0";
    appInfo.locale = "en-US";

    AdblockPlus::PlatformFactory::CreationParameters params;
    params.executor = AdblockPlus::PlatformFactory::CreateExecutor();
    params.fileSystem.reset(new ReadOnlyFileSystem(*params.executor, "data"));
    params.webRequest.reset(new WebRequestNoResponce());

    AdblockPlus::FilterEngineFactory::CreationParameters engineParams;
    engineParams.preconfiguredPrefs.booleanPrefs
        [AdblockPlus::FilterEngineFactory::BooleanPrefName::FirstRunSubscriptionAutoselect] = false;

    platform = AdblockPlus::PlatformFactory::CreatePlatform(std::move(params));
    platform->SetUp(appInfo);
    platform->CreateFilterEngineAsync(engineParams);
  }

  AdblockPlus::JsEngine& GetJsEngine()
  {
    return static_cast<AdblockPlus::DefaultPlatform*>(platform.get())->GetJsEngine();
  }

  AdblockPlus::IFilterEngine& GetFilterEngine() const
  {
    return platform->GetFilterEngine();
  }

  void MatchFromFile(const std::string& file)
  {
    std::ifstream stream(file);
    std::string line;

    while (std::getline(stream, line))
      if (!line.empty())
        MatchRecorded(line);
  }

  void MatchRecorded(const std::string& json)
  {
    auto& engine = GetJsEngine();
    AdblockPlus::JsValue callInfo =
        engine.Evaluate("str => JSON.parse(str)").Call(engine.NewValue(json));

    std::string fn = callInfo.GetProperty("_fn").AsString();

    if (fn == "check-filter-match")
      CheckFilterMatch(callInfo);
    else if (fn == "block-popup")
      BlockPopup(callInfo);
    else if (fn == "generate-js-css")
      GenerateJsCss(callInfo);
  }

  std::vector<std::string> ToList(const AdblockPlus::JsValue& value) const
  {
    std::vector<std::string> res;

    for (const AdblockPlus::JsValue& it : value.AsList())
      res.push_back(it.AsString());

    return res;
  }

  void GenerateJsCss(const AdblockPlus::JsValue& info) const
  {
    auto& engine = GetFilterEngine();
    auto url = info.GetProperty("gurl").AsString();
    auto process_id = info.GetProperty("process_id").AsInt();
    auto frame_id = info.GetProperty("frame_id").AsInt();
    auto documentUrls = ToList(info.GetProperty("referrers"));
    auto sitekey = info.GetProperty("sitekey").AsString();

    if (url.rfind("http:", 0) == 0 || url.rfind("https:", 0) == 0)
    {
      if (!engine.IsContentAllowlisted(
              url, AdblockPlus::IFilterEngine::CONTENT_TYPE_DOCUMENT, documentUrls, sitekey) &&
          !engine.IsContentAllowlisted(
              url, AdblockPlus::IFilterEngine::CONTENT_TYPE_ELEMHIDE, documentUrls, sitekey))
      {
        if (process_id >= 0 && frame_id >= 0)
        {
          engine.GetElementHidingEmulationSelectors(url);
          engine.GetElementHidingStyleSheet(
              url,
              engine.IsContentAllowlisted(
                  url, AdblockPlus::IFilterEngine::CONTENT_TYPE_GENERICHIDE, documentUrls));
        }
      }
    }
  }

  void BlockPopup(const AdblockPlus::JsValue& info) const
  {
    auto& engine = GetFilterEngine();
    auto url = info.GetProperty("url").AsString();
    auto opener = info.GetProperty("opener").AsString();
    PopupBlockResult result = PopupBlockResult::NO_RULE;

    AdblockPlus::Filter filter =
        engine.Matches(url, AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_POPUP, opener);

    if (filter.IsValid())
    {
      if (filter.GetType() != AdblockPlus::Filter::Type::TYPE_EXCEPTION)
        result = PopupBlockResult::BLOCK_RULE;
      else
        result = PopupBlockResult::ALLOW_RULE;
    }
    else
      result = PopupBlockResult::NO_RULE;

    EXPECT_EQ(info.GetProperty("_res").AsInt(), static_cast<int>(result));
  }

  void CheckFilterMatch(const AdblockPlus::JsValue& info) const
  {
    auto& engine = GetFilterEngine();
    auto url = info.GetProperty("request_url").AsString();
    auto documentUrls = ToList(info.GetProperty("referrers"));
    auto sitekey = info.GetProperty("sitekey").AsString();
    AdblockPlus::IFilterEngine::ContentType contentTypeMask =
        static_cast<AdblockPlus::IFilterEngine::ContentType>(
            info.GetProperty("adblock_resource_type").AsInt());
    bool result = false;

    if (engine.IsContentAllowlisted(url,
                                    AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_DOCUMENT,
                                    documentUrls,
                                    sitekey))
    {
      result = false;
    }
    else
    {
      bool specificOnly = false;

      if (!documentUrls.empty())
      {
        specificOnly = engine.IsContentAllowlisted(
            url,
            AdblockPlus::IFilterEngine::ContentType::CONTENT_TYPE_GENERICBLOCK,
            documentUrls,
            sitekey);
      }

      AdblockPlus::Filter filter;

      if (documentUrls.empty())
        filter = engine.Matches(url, contentTypeMask, "", sitekey, specificOnly);
      else
        filter = engine.Matches(url, contentTypeMask, documentUrls.front(), sitekey, specificOnly);

      result = filter.IsValid() && filter.GetType() != AdblockPlus::Filter::Type::TYPE_EXCEPTION;
    }

    EXPECT_EQ(info.GetProperty("_res").AsInt(), result);
  }
};

TEST_F(HarnessTest, FilterMatch)
{
  MatchRecorded(
      "{\"_fn\":\"check-filter-match\",\"_res\":false,\"_time\":2.072,\"adblock_resource_type\":8,"
      "\"initiator_url\":\"https://en.m.wikipedia.org/\",\"process_id\":3,\"referrers\":[\"https:/"
      "/en.m.wikipedia.org/wiki/Main_Page\"],\"render_frame_id\":1,\"request_url\":\"https://"
      "en.m.wikipedia.org/w/"
      "load.php?lang=en&modules=ext.wikimediaBadges%7Cmediawiki.hlist%7Cmediawiki.ui."
      "button%2Cicon%7Cmobile.init.styles%7Cskins.minerva.base.styles%7Cskins.minerva."
      "content.styles%7Cskins.minerva.content.styles.images%7Cskins.minerva.icons."
      "wikimedia%7Cskins.minerva.mainMenu.icons%2Cstyles%7Cskins.minerva.mainPage.styles&"
      "only=styles&skin=minerva\",\"resource_type\":2,\"sitekey\":\"\"}");
}

TEST_F(HarnessTest, GenerateJsCss)
{
  MatchRecorded("{\"_fn\":\"generate-js-css\",\"_time\":2.022,\"frame_id\":1,\"gurl\":\"https://"
                "en.m.wikipedia.org/wiki/Main_Page\",\"process_id\":3,\"referrers\":[\"https://"
                "en.m.wikipedia.org/wiki/Main_Page\"],\"sitekey\":\"\"}");
}

TEST_F(HarnessTest, FilterMatchFromFile)
{
  MatchFromFile("data/rec.log");
}
