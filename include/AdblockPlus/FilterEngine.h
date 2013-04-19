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

  class JsObject : public JsValue
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

  protected:
    JsObject(JsValuePtr value);
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

    Filter(JsValuePtr value);
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

    Subscription(JsValuePtr value);
  };

  typedef std::tr1::shared_ptr<Filter> FilterPtr;
  typedef std::tr1::shared_ptr<Subscription> SubscriptionPtr;

  class FilterEngine
  {
  public:
    explicit FilterEngine(JsEnginePtr jsEngine);
    FilterPtr GetFilter(const std::string& text);
    SubscriptionPtr GetSubscription(const std::string& url);
    std::vector<FilterPtr> GetListedFilters() const;
    std::vector<SubscriptionPtr> GetListedSubscriptions() const;
    std::vector<SubscriptionPtr> FetchAvailableSubscriptions() const;
    FilterPtr Matches(const std::string& url,
        const std::string& contentType,
        const std::string& documentUrl);
    std::vector<std::string> GetElementHidingSelectors(const std::string& domain) const;

  private:
    JsEnginePtr jsEngine;
  };
}

#endif
