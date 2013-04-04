#ifndef ADBLOCKPLUS_FILTER_ENGINE_H
#define ADBLOCKPLUS_FILTER_ENGINE_H

#include <vector>
#include <map>
#include <string>

namespace AdblockPlus
{
  class JsEngine;
  class FilterEngine;

  class Subscription
  {
    friend class FilterEngine;
  public:
    const std::string GetProperty(const std::string& name) const;
    int GetIntProperty(const std::string& name) const;
    void SetProperty(const std::string& name, const std::string& value);
    void SetIntProperty(const std::string& name, int value);

    bool IsListed() const;
    void AddToList();
    void RemoveFromList();
    void UpdateFilters();

  private:
#if FILTER_ENGINE_STUBS
    Subscription(FilterEngine& filterEngine, const std::string& url);

    FilterEngine& filterEngine;
    std::map<std::string,std::string> properties;
    std::map<std::string,int> intProperties;
#else
    Subscription();
#endif
  };

  typedef void (*SubscriptionsCallback)(const std::vector<Subscription*>&);

  class FilterEngine
  {
    friend class Subscription;
  public:
    explicit FilterEngine(JsEngine& jsEngine);
    Subscription& GetSubscription(const std::string& url);
    const std::vector<Subscription*>& GetListedSubscriptions() const;
    void FetchAvailableSubscriptions(SubscriptionsCallback callback);
    bool Matches(const std::string& url,
                 const std::string& contentType,
                 const std::string& documentUrl) const;
    std::vector<std::string> GetElementHidingRules() const;

  private:
    JsEngine& jsEngine;
#if FILTER_ENGINE_STUBS
    std::map<std::string,Subscription*> knownSubscriptions;
    std::vector<Subscription*> listedSubscriptions;
#endif
  };
}

#endif
