#ifndef ADBLOCKPLUS_FILTER_ENGINE_H
#define ADBLOCKPLUS_FILTER_ENGINE_H

#include <vector>
#include <map>
#include <string>

namespace AdblockPlus
{
  class JsEngine;
  class FilterEngine;

  class JSObject
  {
  public:
    std::string GetProperty(const std::string& name, const std::string& defaultValue) const;
    int GetProperty(const std::string& name, int defaultValue) const;
    bool GetProperty(const std::string& name, bool defaultValue) const;
    inline std::string GetProperty(const std::string& name, const char* defaultValue) const
    {
      return GetProperty(name, std::string(defaultValue));
    }

    void SetProperty(const std::string& name, const std::string& value);
    void SetProperty(const std::string& name, int value);
    void SetProperty(const std::string& name, bool value);
    inline void SetProperty(const std::string& name, const char* value)
    {
      SetProperty(name, std::string(value));
    }

  protected:
#if FILTER_ENGINE_STUBS
    JSObject(FilterEngine& filterEngine);

    FilterEngine& filterEngine;
    std::map<std::string, std::string> stringProperties;
    std::map<std::string, int> intProperties;
    std::map<std::string, bool> boolProperties;
#else
    JSObject();
#endif
  };

  class Filter : public JSObject
  {
    friend class FilterEngine;

  public:
    bool IsListed() const;
    void AddToList();
    void RemoveFromList();

  private:
#if FILTER_ENGINE_STUBS
    Filter(FilterEngine& filterEngine, const std::string& text);
#else
    Filter();
#endif
  };

  class Subscription : public JSObject
  {
    friend class FilterEngine;

  public:
    bool IsListed() const;
    void AddToList();
    void RemoveFromList();
    void UpdateFilters();

  private:
#if FILTER_ENGINE_STUBS
    Subscription(FilterEngine& filterEngine, const std::string& url);
#else
    Subscription();
#endif
  };

  typedef void (*SubscriptionsCallback)(const std::vector<Subscription*>&);

  class FilterEngine
  {
    friend class Filter;
    friend class Subscription;
  public:
    explicit FilterEngine(JsEngine& jsEngine);
    Filter& GetFilter(const std::string& text);
    Subscription& GetSubscription(const std::string& url);
    const std::vector<Filter*>& GetListedFilters() const;
    const std::vector<Subscription*>& GetListedSubscriptions() const;
    void FetchAvailableSubscriptions(SubscriptionsCallback callback);
    Filter* Matches(const std::string& url,
                    const std::string& contentType,
                    const std::string& documentUrl);
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;

  private:
    JsEngine& jsEngine;
#if FILTER_ENGINE_STUBS
    std::map<std::string, Filter*> knownFilters;
    std::vector<Filter*> listedFilters;
    std::map<std::string, Subscription*> knownSubscriptions;
    std::vector<Subscription*> listedSubscriptions;
#endif
  };
}

#endif
