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

  class Filter : public JsValue,
                 public std::tr1::enable_shared_from_this<Filter>
  {
  public:
    enum Type {TYPE_BLOCKING, TYPE_EXCEPTION,
               TYPE_ELEMHIDE, TYPE_ELEMHIDE_EXCEPTION,
               TYPE_COMMENT, TYPE_INVALID};

    Type GetType();
    bool IsListed();
    void AddToList();
    void RemoveFromList();
    bool operator==(const Filter& filter) const;

    Filter(JsValuePtr value);
  };

  class Subscription : public JsValue,
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
