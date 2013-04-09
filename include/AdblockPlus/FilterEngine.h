#ifndef ADBLOCKPLUS_FILTER_ENGINE_H
#define ADBLOCKPLUS_FILTER_ENGINE_H

#include <vector>
#include <map>
#include <string>
#ifdef _WIN32 || _WIN64
#include <memory>
#include <cctype>
#include <functional>
#else
#include <tr1/memory>
#endif

namespace AdblockPlus
{
  class JsEngine;
  class FilterEngine;

  class JsObject
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
    JsObject(FilterEngine& filterEngine);

    FilterEngine& filterEngine;
    std::map<std::string, std::string> stringProperties;
    std::map<std::string, int> intProperties;
    std::map<std::string, bool> boolProperties;
#else
    JsObject();
#endif
  };

  class Filter : public JsObject,
                 public std::tr1::enable_shared_from_this<Filter>
  {
    friend class FilterEngine;

  public:
    enum Type {TYPE_BLOCKING, TYPE_EXCEPTION,
               TYPE_ELEMHIDE, TYPE_ELEMHIDE_EXCEPTION,
               TYPE_COMMENT, TYPE_INVALID};

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

  class Subscription : public JsObject,
                       public std::tr1::enable_shared_from_this<Subscription>
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

  typedef std::tr1::shared_ptr<Filter> FilterPtr;
  typedef std::tr1::shared_ptr<Subscription> SubscriptionPtr;
  typedef void (*SubscriptionsCallback)(const std::vector<SubscriptionPtr>&);

  class FilterEngine
  {
    friend class Filter;
    friend class Subscription;
  public:
    explicit FilterEngine(JsEngine& jsEngine);
    Filter& GetFilter(const std::string& text);
    Subscription& GetSubscription(const std::string& url);
    const std::vector<FilterPtr>& GetListedFilters() const;
    const std::vector<SubscriptionPtr>& GetListedSubscriptions() const;
    void FetchAvailableSubscriptions(SubscriptionsCallback callback);
    FilterPtr Matches(const std::string& url,
        const std::string& contentType,
        const std::string& documentUrl);
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;

  private:
    JsEngine& jsEngine;
#if FILTER_ENGINE_STUBS
    std::map<std::string, FilterPtr> knownFilters;
    std::vector<FilterPtr> listedFilters;
    std::map<std::string, SubscriptionPtr> knownSubscriptions;
    std::vector<SubscriptionPtr> listedSubscriptions;
#endif
  };
}

#endif
