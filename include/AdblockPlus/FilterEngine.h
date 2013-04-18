#ifndef ADBLOCKPLUS_FILTER_ENGINE_H
#define ADBLOCKPLUS_FILTER_ENGINE_H

#include <vector>
#include <map>
#include <string>
#include <AdblockPlus/JsEngine.h>
#include <AdblockPlus/JsValue.h>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class FilterEngine;

#if FILTER_ENGINE_STUBS
  class JsObject
#else
  class JsObject : public JsValue
#endif
  {
  public:
    std::string GetProperty(const std::string& name, const std::string& defaultValue) const;
    int64_t GetProperty(const std::string& name, int64_t defaultValue) const;
    bool GetProperty(const std::string& name, bool defaultValue) const;
    inline std::string GetProperty(const std::string& name, const char* defaultValue) const
    {
      return GetProperty(name, std::string(defaultValue));
    }
    inline int64_t GetProperty(const std::string& name, int defaultValue) const
    {
      return GetProperty(name, static_cast<int64_t>(defaultValue));
    }

#if FILTER_ENGINE_STUBS
    void SetProperty(const std::string& name, const std::string& value);
    void SetProperty(const std::string& name, int64_t value);
    void SetProperty(const std::string& name, bool value);
    inline void SetProperty(const std::string& name, const char* value)
    {
      SetProperty(name, std::string(value));
    }
    inline void SetProperty(const std::string& name, int value)
    {
      SetProperty(name, static_cast<int64_t>(value));
    }
#endif

  protected:
#if FILTER_ENGINE_STUBS
    JsObject(FilterEngine& filterEngine);

    FilterEngine& filterEngine;
    std::map<std::string, std::string> stringProperties;
    std::map<std::string, int64_t> intProperties;
    std::map<std::string, bool> boolProperties;
#else
    JsObject(JsValuePtr value);
#endif
  };

  class Filter : public JsObject,
                 public std::tr1::enable_shared_from_this<Filter>
  {
  public:
    enum Type {TYPE_BLOCKING, TYPE_EXCEPTION,
               TYPE_ELEMHIDE, TYPE_ELEMHIDE_EXCEPTION,
               TYPE_COMMENT, TYPE_INVALID};

    bool IsListed();
    void AddToList();
    void RemoveFromList();
    bool operator==(const Filter& filter) const;

#if FILTER_ENGINE_STUBS
  private:
    friend class FilterEngine;
    Filter(FilterEngine& filterEngine, const std::string& text);
#else
    Filter(JsValuePtr value);
#endif
  };

  class Subscription : public JsObject,
                       public std::tr1::enable_shared_from_this<Subscription>
  {
  public:
    bool IsListed();
    void AddToList();
    void RemoveFromList();
    void UpdateFilters();
    bool IsUpdating();
    bool operator==(const Subscription& subscription) const;

#if FILTER_ENGINE_STUBS
  private:
    friend class FilterEngine;
    Subscription(FilterEngine& filterEngine, const std::string& url);
#else
    Subscription(JsValuePtr value);
#endif
  };

  typedef std::tr1::shared_ptr<Filter> FilterPtr;
  typedef std::tr1::shared_ptr<Subscription> SubscriptionPtr;
  typedef void (*SubscriptionsCallback)(const std::vector<SubscriptionPtr>&);

  class FilterEngine
  {
#if FILTER_ENGINE_STUBS
    friend class Filter;
    friend class Subscription;
#endif

  public:
    explicit FilterEngine(JsEnginePtr jsEngine);
    FilterPtr GetFilter(const std::string& text);
    SubscriptionPtr GetSubscription(const std::string& url);
    const std::vector<FilterPtr> GetListedFilters() const;
    const std::vector<SubscriptionPtr> GetListedSubscriptions() const;
    void FetchAvailableSubscriptions(SubscriptionsCallback callback);
    FilterPtr Matches(const std::string& url,
        const std::string& contentType,
        const std::string& documentUrl);
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;

  private:
    JsEnginePtr jsEngine;
#if FILTER_ENGINE_STUBS
    std::map<std::string, FilterPtr> knownFilters;
    std::vector<FilterPtr> listedFilters;
    std::map<std::string, SubscriptionPtr> knownSubscriptions;
    std::vector<SubscriptionPtr> listedSubscriptions;
#endif
  };
}

#endif
